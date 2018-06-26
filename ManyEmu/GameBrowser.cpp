#include <Core/Log.h>
#include "Backend.h"
#include "GameBrowser.h"
#include "GameSession.h"
#include "Path.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#define DUMP_ROM_LIST 0

#if DUMP_ROM_LIST
#include <io.h>
#include "Gameboy/GB.h"
#endif

namespace
{
    std::string getExtension(const std::string& path)
    {
        std::string root;
        std::string ext;
        Path::splitExt(path, root, ext);
        Path::normalizeCase(ext);
        return ext;
    }

    IBackend* getBackend(Application& application, const std::string& path)
    {
        return application.getBackendRegistry().getBackend(getExtension(path).c_str());
    }

    struct GameContext
    {
        bool initialize(Application& application, const std::string& path, const std::string& saveDirectory)
        {
            mApplication = &application;

            auto backend = getBackend(application, path);
            EMU_VERIFY(backend);

            mSession = std::make_unique<GameSession>();
            EMU_VERIFY(mSession->loadRom(*backend, path, saveDirectory));
            mSession->loadGameData();

            const auto& displayInfo = mSession->getDisplayInfo();
            mSizeX = displayInfo.sizeX;
            mSizeY = displayInfo.sizeY;
            mPitch = mSizeX * 4;
            mRenderBuffer.resize(mSizeY * mPitch);
            mTexture = mApplication->getGraphics().createTexture(mSizeX, mSizeY);
            EMU_VERIFY(mTexture);

            return true;
        }

        ~GameContext()
        {
            if (mTexture) mApplication->getGraphics().destroyTexture(mTexture);
        }

        void update()
        {
            mSession->setSoundBuffer(nullptr, 0);
            mSession->setRenderBuffer(mRenderBuffer.data(), mPitch);
            mSession->execute();
            mSession->setRenderBuffer(nullptr, 0);
            mApplication->getGraphics().updateTexture(*mTexture, mRenderBuffer.data(), mRenderBuffer.size());
        }

        Application*                    mApplication = nullptr;
        std::unique_ptr<GameSession>    mSession;
        uint32_t                        mSizeX = 0;
        uint32_t                        mSizeY = 0;
        uint32_t                        mPitch = 0;
        std::vector<uint8_t>            mRenderBuffer;
        ITexture*                       mTexture = nullptr;
    };

    class MultipleGamesView : public Application::IView
    {
    public:
        MultipleGamesView()
        {
        }

        bool initialize(IGraphics& graphics, const std::string& name)
        {
            mGraphics = &graphics;
            mName = name;
            return true;
        }

        void addContext(GameContext& context)
        {
            if (std::count(std::begin(mContexts), std::end(mContexts), &context) == 0)
                mContexts.push_back(&context);
        }

        void removeContext(GameContext& context)
        {
            mContexts.erase(std::remove(std::begin(mContexts), std::end(mContexts), &context), std::end(mContexts));
        }

        void clearContext()
        {
            mContexts.clear();
        }

        const char* getName() const override
        {
            return mName.c_str();
        }

        void onGUI()
        {
            ImGui::BeginChild("Games");
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            int imageSize = 128;
            int count = static_cast<int>(mContexts.size());

            ImGuiStyle& style = ImGui::GetStyle();
            int spacing = static_cast<int>(style.ItemSpacing.x);

            int sizeX = static_cast<int>(ImGui::GetContentRegionAvailWidth());
            int countX = (sizeX + spacing) / (imageSize + spacing);
            countX = std::max(countX, 1);
            int countY = (count + countX - 1) / countX;
            ImGuiListClipper clipper(countY);

            while (clipper.Step())
            {
                for (int indexY = clipper.DisplayStart; indexY < clipper.DisplayEnd; ++indexY)
                {
                    for (int indexX = 0; indexX < countX; ++indexX)
                    {
                        int index = indexX + indexY * countX;
                        if (index >= count) continue;
                        if (indexX > 0)
                            ImGui::SameLine();

                        ImGui::BeginGroup();

                        // Game image
                        auto& context = *mContexts[index];
                        auto textureID = mGraphics->imGuiRenderer_getTextureID(*context.mTexture);
                        ImGui::Image(textureID, ImVec2(static_cast<float>(imageSize), static_cast<float>(imageSize)));
                        
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();

                            // Scaling for focus preview
                            auto maxAxis = std::max(context.mSizeX, context.mSizeY);
                            auto size = std::max(imageSize * 1.5f, maxAxis * 1.0f);
                            auto scale = size / maxAxis;

                            // Game preview
                            ImDrawList* drawList = ImGui::GetWindowDrawList();
                            auto previewSize = ImVec2(context.mSizeX * scale, context.mSizeY * scale);
                            auto screenPos = ImGui::GetCursorScreenPos();
                            auto screenEnd = ImVec2(screenPos.x + previewSize.x, screenPos.y + previewSize.y);
                            ImU32 backgroundColor = ImColor(0, 0, 0, 255);
                            drawList->AddRectFilled(screenPos, screenEnd, backgroundColor);
                            ImGui::Image(textureID, previewSize);

                            // Game name
                            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + context.mSizeX * scale);
                            ImGui::Text(context.mSession->getRomName().c_str());
                            ImGui::PopTextWrapPos();
                            
                            ImGui::EndTooltip();
                        }

                        ImGui::EndGroup();
                    }
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndChild();
        }

    private:
        IGraphics*                  mGraphics = nullptr;
        std::string                 mName;
        std::vector<GameContext*>   mContexts;
    };

    class GameBrowserImpl : public GameBrowser
    {
    public:
        struct Config
        {
            typedef std::vector<std::string> StringArray;

            StringArray     roms;               // ROM files to execute. Only the first rom specified is playable for now.
            std::string     romFolder;          // Location where the ROM files are to be found.
            std::string     saveFolder;         // Savegame folder
        };

        bool create(Application& application) override
        {
            mView = std::make_unique<MultipleGamesView>();
            EMU_VERIFY(mView->initialize(application.getGraphics(), getName()));
            application.addView(*mView.get());

            overrideConfig();

            for (auto rom : mConfig.roms)
            {
                auto context = std::make_unique<GameContext>();
                if (context->initialize(application, Path::join(mConfig.romFolder, rom), mConfig.saveFolder))
                {
                    mGameContexts.push_back(std::move(context));
                    mView->addContext(*mGameContexts.back().get());
                }
            }

            return true;
        }

        void destroy(Application& application) override
        {
            mGameContexts.clear();
            if (mView) application.removeView(*mView.get());
        }

        const char* getName() const override
        {
            return "Game Browser";
        }

        void update() override
        {
            for (auto& context : mGameContexts)
                context->update();
        }

    private:
        void overrideConfig()
        {
            auto& config = mConfig;
#if 1
            // Configuration for Gameboy development
            config.saveFolder = "C:\\Emu\\Gameboy\\save";
            config.romFolder = "C:\\Emu\\Gameboy\\roms";
            config.roms.push_back("Mario Tennis (U) [C][!].gbc");
            config.roms.push_back("p-shntae.gbc");
            config.roms.push_back("rayman (u) [c][t1].gbc");
            config.roms.push_back("Asterix - Search for Dogmatix (E) (M6) [C][!].gbc");
            config.roms.push_back("3D Pocket Pool (E) (M6) [C][!].gbc");
            config.roms.push_back("super mario land (v1.1) (jua) [t1].gb");
            config.roms.push_back("Tetris (V1.1) (JU) [!].gb");
            config.roms.push_back("Metroid 2 - Return of Samus (UA) [b1].gb");
            config.roms.push_back("Legend of Zelda, The - Link's Awakening (V1.2) (U) [!].gb");
#else
            // Configuration for NES development
            config.saveFolder = "C:\\Emu\\NES\\save";
            config.romFolder = "C:\\Emu\\NES\\roms";
            config.roms.push_back("smb3.nes");
            config.roms.push_back("exitbike.nes");
            config.roms.push_back("megaman2.nes");
            config.roms.push_back("mario.nes");
            config.roms.push_back("zelda2.nes");
#endif

#if DUMP_ROM_LIST
            std::string filter = Path::join(config.romFolder, "*.gb*");
            _finddata_t data;
            auto handle = _findfirst(filter.c_str(), &data);
            int valid = 1;
            emu::Log::printf(emu::Log::Type::Debug, "File;ROM size;RAM size;Title;UseCGB;OnlyCGB;UseSGB;RAM;Battery;Timer;Rumble;Mapper;Destination;Cartridge type;Version;Licensee old;Licensee new;Manufacturer;Header checksum;Global checksum\n");
            while (handle && valid)
            {
                std::string path = Path::join(config.romFolder, data.name);
                gb::Rom::Description desc;
                gb::Rom::readDescription(desc, path.c_str());
                emu::Log::printf(emu::Log::Type::Debug, "%s;%d;%d;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;0x%02x;%d;0x%02x;0x%04x;0x%08x;0x%02x;0x%04x\n",
                    data.name,
                    desc.romSize,
                    desc.ramSize,
                    desc.title,
                    desc.useCGB,
                    desc.onlyCGB,
                    desc.useSGB,
                    desc.hasRam,
                    desc.hasBattery,
                    desc.hasTimer,
                    desc.hasRumble,
                    gb::Rom::getMapperName(desc.mapper),
                    gb::Rom::getDestinationName(desc.destination),
                    desc.cartridgeType,
                    desc.version,
                    desc.licenseeOld,
                    desc.licenseeNew,
                    desc.manufacturer,
                    desc.headerChecksum,
                    desc.globalChecksum);
                valid = !_findnext(handle, &data);
            }
#endif
        }

        typedef std::vector<std::unique_ptr<GameContext>> GameContextArray;

        Config                              mConfig;
        GameContextArray                    mGameContexts;
        std::unique_ptr<MultipleGamesView>  mView;
    };
}

GameBrowser* GameBrowser::create()
{
    return new GameBrowserImpl();
}

void GameBrowser::destroy(GameBrowser& instance)
{
    delete &static_cast<GameBrowserImpl&>(instance);
}
