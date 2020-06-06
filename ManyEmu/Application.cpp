#include "Application.h"
#include "Backend.h"
#include "GameView.h"
#include "ImGuiContext.h"
#include "LogView.h"
#include "ThreadPool.h"
#include <Core/YamlSerializer.h>
#include <SDL.h>
#include <algorithm>
#include <map>
#include <vector>

using namespace emu;

namespace
{
    bool readFile(std::string& buffer, const char* path)
    {
        auto file = fopen(path, "rb");
        EMU_VERIFY(file);
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer.resize(size);
        bool success = fread(&buffer.at(0), size, 1, file) == 1;
        fclose(file);
        return success;
    }

    bool writeFile(const std::string& buffer, const char* path)
    {
        auto file = fopen(path, "wb");
        EMU_VERIFY(file);
        bool success = fwrite(buffer.data(), buffer.size(), 1, file) == 1;
        fclose(file);
        return success;
    }

    class ApplicationImpl : public Application
    {
    public:
        struct Settings
        {
            Settings()
                : x(-1)
                , y(-1)
                , width(1280)
                , height(720)
                , showImGuiTestView(false)
            {
            }

            int32_t     x;
            int32_t     y;
            int32_t     width;
            int32_t     height;
            bool        showImGuiTestView;

            void serialize(emu::ISerializer& serializer)
            {
                serializer
                    .value("x", x)
                    .value("y", y)
                    .value("width", width)
                    .value("height", height)
                    .value("showImGuiTestView", showImGuiTestView);
            }
        };

        ApplicationImpl()
            : mThreadPool(std::thread::hardware_concurrency())
        {
            initialize();
            create();
        }

        ~ApplicationImpl()
        {
            destroy();
        }

        virtual bool run() override
        {
            LogView::Enabled enabled(getLogView());

            EMU_VERIFY(mRunning);
            while (mRunning)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    ImGuiContext_ProcessEvent(&event);
                    if (event.type == SDL_QUIT)
                        mRunning = false;
                }

                update();
                render();
            }
            saveSettings();
            return true;
        }

        virtual IGraphics& getGraphics() override
        {
            return *mGraphics;
        }

        virtual void addPlugin(IPlugin& plugin) override
        {
            mPlugins.push_back(&plugin);
            if (!plugin.create(*this))
                removePlugin(plugin);
        }

        virtual void removePlugin(IPlugin& plugin) override
        {
            auto item = std::find(mPlugins.begin(), mPlugins.end(), &plugin);
            if (item != mPlugins.end())
            {
                plugin.destroy(*this);
                mPlugins.erase(item);
            }
        }

        virtual void addView(IView& view) override
        {
            mViews.push_back(&view);
            mViewSettings[view.getName()];
        }

        virtual void removeView(IView& view) override
        {
            auto item = std::find(mViews.begin(), mViews.end(), &view);
            if (item != mViews.end())
            {
                mViews.erase(item);
            }
        }

        virtual GameView& getGameView() override
        {
            return mGameView;
        }

        virtual LogView& getLogView() override
        {
            return mLogView;
        }

        virtual BackendRegistry& getBackendRegistry() override
        {
            return mBackendRegistry;
        }

        virtual ThreadPool& getThreadPool() override
        {
            return mThreadPool;
        }

    private:
        void initialize()
        {
            mRunning = false;
            mWindow = nullptr;
            mGraphics = nullptr;
        }

        bool create()
        {
            SDL_Init(SDL_INIT_EVERYTHING);

            loadSettings();

            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(0, &current);

            int windowX = mSettings.x >= 0 ? mSettings.x : SDL_WINDOWPOS_CENTERED;
            int windowY = mSettings.y >= 0 ? mSettings.y : SDL_WINDOWPOS_CENTERED;
            int windowWidth = mSettings.width;
            int windowHeight = mSettings.height;
            mWindow = SDL_CreateWindow("ManyEmu", windowX, windowY, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
            EMU_VERIFY(mWindow);

            mGraphics = IGraphics::create(*mWindow);
            EMU_VERIFY(mGraphics);

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
            //io.ConfigViewportsNoAutoMerge = true;
            //io.ConfigViewportsNoTaskBarIcon = true;

            // Initialize ImGui
            EMU_VERIFY(ImGuiContext_Init(mWindow, *mGraphics));

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsClassic();

            // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
            ImGuiStyle& style = ImGui::GetStyle();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }

            // Load Fonts
            io.Fonts->AddFontDefault();
            io.Fonts->AddFontFromFileTTF("fonts\\DroidSans.ttf", 14.0f);
            io.Fonts->AddFontFromFileTTF("fonts\\Roboto-Medium.ttf", 16.0f);
            io.Fonts->AddFontFromFileTTF("fonts\\Cousine-Regular.ttf", 15.0f);
            io.Fonts->AddFontFromFileTTF("fonts\\ProggyTiny.ttf", 10.0f);
            io.Fonts->AddFontFromFileTTF("fonts\\Karla-Regular.ttf", 14.0f);

            addView(mGameView);
            addView(mLogView);

            mRunning = true;
            return true;
        }

        void destroy()
        {
            removeView(mGameView);
            removeView(mLogView);

            while (!mViews.empty())
            {
                removeView(*mViews.back());
            }

            while (!mPlugins.empty())
            {
                removePlugin(*mPlugins.back());
            }

            ImGuiContext_Shutdown();
            IGraphics::destroy(mGraphics);
            if (mWindow) SDL_DestroyWindow(mWindow);
            SDL_Quit();
            initialize();
        }

        bool serializeConfig(emu::ISerializer& serializer)
        {
            serializer.value("Settings", mSettings);
            if (serializer.nodeBegin("Views"))
            {
                for (auto viewSettings : mViewSettings)
                {
                    serializer.value(viewSettings.first.c_str(), viewSettings.second);
                }
                serializer.nodeEnd();
            }
            if (serializer.nodeBegin("Plugins"))
            {
                for (auto plugin : mPlugins)
                {
                    if (serializer.nodeBegin(plugin->getName()))
                    {
                        plugin->serialize(serializer);
                        serializer.nodeEnd();
                    }
                }
                serializer.nodeEnd();
            }
            return true;
        }

        const char* getSettingsPath()
        {
            return "Settings.yml";
        }

        void loadSettings()
        {
            std::string data;
            if (readFile(data, getSettingsPath()))
            {
                auto serializer = YamlReader::create();
                if (serializer)
                {
                    if (serializer->read(data))
                    {
                        serializeConfig(*serializer);
                    }
                    YamlReader::destroy(*serializer);
                }
            }
        }

        void saveSettings()
        {
            int x = -1, y = -1, width = 0, height = 0;
            SDL_GetWindowPosition(mWindow, &x, &y);
            SDL_GetWindowSize(mWindow, &width, &height);
            mSettings.x = x;
            mSettings.y = y;
            mSettings.width = width;
            mSettings.height = height;

            auto serializer = YamlWriter::create();
            if (serializer)
            {
                serializeConfig(*serializer);

                std::string data;
                if (serializer->write(data))
                {
                    writeFile(data, getSettingsPath());
                }
                YamlWriter::destroy(*serializer);
            }
        }

        void update()
        {
            for (auto item : mPlugins)
            {
                item->update();
            }

            ImGuiContext_NewFrame(mWindow);

            // Begin docking
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
            ImGui::Begin("DockSpace", nullptr, windowFlags);
            ImGui::PopStyleVar(3);
            ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f), dockspaceFlags);

            float menuHeight = 0.0f;
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Exit"))
                        mRunning = false;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View"))
                {
                    for (const auto& view : mViews)
                    {
                        auto& viewSettings = mViewSettings[view->getName()];
                        ImGui::MenuItem(view->getName(), nullptr, &viewSettings.visible);
                    }

                    ImGui::Separator();
                    ImGui::MenuItem("ImGUI test window", nullptr, &mSettings.showImGuiTestView);
                    ImGui::EndMenu();
                }
                menuHeight = ImGui::GetWindowSize().y;
                ImGui::EndMainMenuBar();
            }

            if (mSettings.showImGuiTestView)
            {
                ImGui::ShowDemoWindow(&mSettings.showImGuiTestView);
            }

            for (auto view : mViews)
            {
                const char* name = view->getName();
                ViewSettings& viewSettings = mViewSettings[name];
                if (viewSettings.visible)
                {
                    ImGui::SetNextWindowSize(ImVec2(640.0f, 360.0f), ImGuiCond_FirstUseEver);
                    if (ImGui::Begin(name, &viewSettings.visible))
                    {
                        view->onGUI();
                    }
                    ImGui::End();
                }
            }

            for (auto item : mPlugins)
            {
                item->onGUI();
            }

            // End docking
            ImGui::End();
        }

        void render()
        {
            ImGuiContext_Draw();
            mGraphics->swapBuffers();
        }

        struct ViewSettings
        {
            ViewSettings()
                : visible(true)
            {
            }

            void serialize(emu::ISerializer& serializer)
            {
                serializer
                    .value("Visible", visible);
            }

            bool                                visible;
        };

        ThreadPool                              mThreadPool;
        Settings                                mSettings;
        bool                                    mRunning;
        SDL_Window*                             mWindow;
        IGraphics*                              mGraphics;
        std::vector<IPlugin*>                   mPlugins;
        std::vector<IView*>                     mViews;
        std::map<std::string, ViewSettings>     mViewSettings;

        GameView                                mGameView;
        LogView                                 mLogView;

        BackendRegistry                         mBackendRegistry;
    };
}

Application* Application::create()
{
    return new ApplicationImpl();
}

void Application::destroy(Application& instance)
{
    delete &static_cast<ApplicationImpl&>(instance);
}