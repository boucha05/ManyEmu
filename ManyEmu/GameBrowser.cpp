#include <Core/Log.h>
#include "Backend.h"
#include "GameBrowser.h"
#include "GameSession.h"
#include "Path.h"
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
    class MultipleGamesView : public Application::IView
    {
    public:
        MultipleGamesView()
        {
        }

        bool initialize(const std::string& name)
        {
            mName = name;
            return true;
        }

        const char* getName() const override
        {
            return mName.c_str();
        }

        void onGUI()
        {
        }

    private:
        std::string     mName;
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
            mGraphics = &application.getGraphics();

            overrideConfig();

            Path::makeDirs(mConfig.saveFolder);

            mTexSizeX = 256;
            mTexSizeY = 240;
            mTexture = mGraphics->createTexture(mTexSizeX, mTexSizeY);
            EMU_VERIFY(mTexture);
            mFakeTexture.resize(mTexSizeX * mTexSizeY * 4);

            for (auto rom : mConfig.roms)
            {
                auto gameSession = createGameSession(application, Path::join(mConfig.romFolder, rom), mConfig.saveFolder);
                EMU_VERIFY(gameSession);
                mGameSessions.push_back(gameSession);
            }

            mView = std::make_unique<MultipleGamesView>();
            EMU_VERIFY(mView->initialize(getName()));
            application.addView(*mView.get());

            return true;
        }

        void destroy(Application& application) override
        {
            if (mView) application.removeView(*mView.get());

            for (auto gameSession : mGameSessions)
                destroyGameSession(*gameSession);
            mGameSessions.clear();
            mGraphics->destroyTexture(mTexture);
            mTexture = nullptr;
            mGraphics = nullptr;
        }

        const char* getName() const override
        {
            return "Game Browser";
        }

        void update() override
        {
            if (mGameSessions.size() == 0)
                return;

            auto& gameSession = *mGameSessions[0];
            if (!gameSession.isValid())
                return;

            void* pixels = nullptr;
            int pitch = 0;
            gameSession.setRenderBuffer(nullptr, 0);
            if (true)
            {
                pixels = mFakeTexture.data();
                pitch = mTexSizeX * 4;
                gameSession.setRenderBuffer(mFakeTexture.data(), mTexSizeX * 4);
            }

            gameSession.setSoundBuffer(nullptr, 0);

            gameSession.execute();

            if (pixels)
            {
                mGraphics->updateTexture(*mTexture, pixels, mFakeTexture.size());
            }
        }

    private:
        GameSession* createGameSession(Application& application, const std::string& path, const std::string& saveDirectory)
        {
            std::string root;
            std::string ext;
            Path::splitExt(path, root, ext);
            Path::normalizeCase(ext);
            auto backend = application.getBackendRegistry().getBackend(ext.c_str());
            if (!backend)
                return nullptr;

            auto& gameSession = *new GameSession();
            if (!gameSession.loadRom(*backend, path, saveDirectory))
            {
                delete &gameSession;
                return nullptr;
            }
            gameSession.loadGameData();
            return &gameSession;
        }

        void destroyGameSession(GameSession& gameSession)
        {
            gameSession.unloadRom();
            delete &gameSession;
        }


        void overrideConfig()
        {
            auto& config = mConfig;
#if 1
            // Configuration for Gameboy development
            config.saveFolder = "C:\\Emu\\Gameboy\\save";
            config.romFolder = "C:\\Emu\\Gameboy\\roms";
            //config.roms.push_back("mario & yoshi (e) [o2].gb");
            //config.roms.push_back("Donkey Kong Country (UE) (M5) [C][t1].gbc");
            //config.roms.push_back("Legend of Zelda, The - Oracle of Ages (U) [C][h2].gbc");
            //config.roms.push_back("Donkey Kong Land 2 (UE) [S][b2].gb");
            config.roms.push_back("Mario Tennis (U) [C][!].gbc");
            //config.roms.push_back("p-shntae.gbc");
            //config.roms.push_back("rayman (u) [c][t1].gbc");
            //config.roms.push_back("Asterix - Search for Dogmatix (E) (M6) [C][!].gbc");
            //config.roms.push_back("3D Pocket Pool (E) (M6) [C][!].gbc");
            //config.roms.push_back("super mario land (v1.1) (jua) [t1].gb");
            //config.roms.push_back("Tetris (V1.1) (JU) [!].gb");
            //config.roms.push_back("Metroid 2 - Return of Samus (UA) [b1].gb");
            //config.roms.push_back("Legend of Zelda, The - Link's Awakening (V1.2) (U) [!].gb");
#else
            // Configuration for NES development
            config.saveFolder = "C:\\Emu\\NES\\save";
            config.romFolder = "C:\\Emu\\NES\\roms";
            //config.roms.push_back("smb3.nes");
            //config.roms.push_back("exitbike.nes");
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

        typedef std::vector<GameSession*> GameSessionArray;

        Config                              mConfig;
        IGraphics*                          mGraphics = nullptr;
        uint32_t                            mTexSizeX = 0;
        uint32_t                            mTexSizeY = 0;
        ITexture*                           mTexture = nullptr;
        std::vector<uint8_t>                mFakeTexture;
        GameSessionArray                    mGameSessions;
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
