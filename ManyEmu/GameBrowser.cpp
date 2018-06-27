#include <Core/Log.h>
#include "Backend.h"
#include "GameBrowser.h"
#include "GameSession.h"
#include "Path.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

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
            std::string     romFolder;          // Location where the ROM files are to be found.
            std::string     saveFolder;         // Savegame folder
        };

        bool create(Application& application) override
        {
            mApplication = &application;
            mView = std::make_unique<MultipleGamesView>();
            EMU_VERIFY(mView->initialize(application.getGraphics(), getName()));
            application.addView(*mView.get());

            overrideConfig();

            loadFolder(mConfig.romFolder);

            return true;
        }

        void destroy(Application& application) override
        {
            clearContexts();
            if (mView) application.removeView(*mView.get());
            mApplication = nullptr;
        }

        bool loadFolder(const std::string& folder)
        {
            Path::FileList fileList;
            EMU_VERIFY(Path::walkFiles(fileList, folder, "", "*.*", false));
            for (auto file : fileList)
            {
                std::string path = Path::join(folder, file);
                auto context = std::make_unique<GameContext>();
                if (context->initialize(*mApplication, path, mConfig.saveFolder))
                {
                    mGameContexts.push_back(std::move(context));
                    mView->addContext(*mGameContexts.back().get());
                }
            }
            return true;
        }

        void clearContexts()
        {
            mGameContexts.clear();
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
#else
            // Configuration for NES development
            config.saveFolder = "C:\\Emu\\NES\\save";
            config.romFolder = "C:\\Emu\\NES\\roms";
#endif
        }

        typedef std::vector<std::unique_ptr<GameContext>> GameContextArray;

        Config                              mConfig;
        Application*                        mApplication = nullptr;
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
