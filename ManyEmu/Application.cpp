#define SDL_MAIN_HANDLED
#include "Application.h"
#include <Core/YamlSerializer.h>
#include "imgui_impl_sdl_gl3.h"
#include <SDL.h>
#include <gl/gl3w.h>
#include <algorithm>
#include <vector>

using namespace emu;

namespace
{
    class ImGuiSerializer : public ImGui::ISerializer
    {
    public:
        ImGuiSerializer(emu::ISerializer& serializer)
            : mSerializer(serializer)
            , mTextSize(0)
        {
        }

        ~ImGuiSerializer()
        {
        }

        virtual bool isReading() override
        {
            return mSerializer.isReading();
        }

        virtual void serialize(bool& value, const char* name) override
        {
            mSerializer.value(name, value);
        }

        virtual void serializeTextSize(size_t& value, const char* name) override
        {
            if (mSerializer.isReading())
            {
                mSerializer.value(name, mText);
                mTextSize = value = static_cast<uint32_t>(mText.size());
            }
            else
            {
                mTextSize = value;
            }
        }

        virtual void serializeTextData(char* value, const char* name) override
        {
            if (mSerializer.isReading())
            {
                memcpy(value, mText.c_str(), mTextSize);
            }
            else
            {
                mText.resize(static_cast<int32_t>(mTextSize));
                if (mTextSize > 0)
                    memcpy(&mText.at(0), value, mTextSize);
                mSerializer.value(name, mText);
            }
        }

        virtual void serialize(float& value, const char* name) override
        {
            mSerializer.value(name, value);
        }

        virtual void serialize(int32_t& value, const char* name) override
        {
            mSerializer.value(name, value);
        }

        virtual bool serializeSequenceBegin(size_t& size, const char* name) override
        {
            auto success = mSerializer.nodeBegin(name);
            if (success)
            {
                success = mSerializer.sequenceBegin(size);
                if (!success || !size)
                {
                    mSerializer.nodeEnd();
                }
            }
            if (!success)
            {
                size = 0;
            }
            return success;
        }

        virtual void serializeSequenceEnd() override
        {
            mSerializer.sequenceEnd();
            mSerializer.nodeEnd();
        }

        virtual void serializeSequenceItem() override
        {
            mSerializer.sequenceItem();
        }

    private:
        emu::ISerializer&           mSerializer;
        size_t                      mTextSize;
        std::string                 mText;
    };

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
            EMU_VERIFY(mRunning);
            while (mRunning)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    ImGui_ImplSdlGL3_ProcessEvent(&event);
                    if (event.type == SDL_QUIT)
                        mRunning = false;
                }

                update();
                render();
            }
            saveSettings();
            return true;
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

    private:
        void initialize()
        {
            mRunning = false;
            mWindow = nullptr;
            mGLContext = nullptr;
        }

        bool create()
        {
            SDL_Init(SDL_INIT_EVERYTHING);

            loadSettings();

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            SDL_DisplayMode current;
            SDL_GetCurrentDisplayMode(0, &current);

            int windowX = mSettings.x >= 0 ? mSettings.x : SDL_WINDOWPOS_CENTERED;
            int windowY = mSettings.y >= 0 ? mSettings.y : SDL_WINDOWPOS_CENTERED;
            int windowWidth = mSettings.width;
            int windowHeight = mSettings.height;
            mWindow = SDL_CreateWindow("RomViewer", windowX, windowY, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
            EMU_VERIFY(mWindow);

            mGLContext = SDL_GL_CreateContext(mWindow);
            gl3wInit();

            // Setup ImGui binding
            ImGui_ImplSdlGL3_Init(mWindow);

            mRunning = true;
            return true;
        }

        void destroy()
        {
            while (!mPlugins.empty())
            {
                removePlugin(*mPlugins.back());
            }

            ImGui_ImplSdlGL3_Shutdown();
            if (mGLContext) SDL_GL_DeleteContext(mGLContext);
            if (mWindow) SDL_DestroyWindow(mWindow);
            SDL_Quit();
            initialize();
        }

        bool serializeConfig(emu::ISerializer& serializer)
        {
            serializer.value("Settings", mSettings);
            if (serializer.nodeBegin("Layout"))
            {
                ImGuiSerializer imGuiSerializer(serializer);
                ImGui::SerializeDock(imGuiSerializer);
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

            ImGui_ImplSdlGL3_NewFrame(mWindow);

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
                    ImGui::MenuItem("ImGUI test window", nullptr, &mSettings.showImGuiTestView);
                    ImGui::EndMenu();
                }
                menuHeight = ImGui::GetWindowSize().y;
                ImGui::EndMainMenuBar();
            }

            if (ImGui::GetIO().DisplaySize.y > 0)
            {
                auto pos = ImVec2(0, menuHeight);
                auto size = ImGui::GetIO().DisplaySize;
                size.y -= pos.y;
                ImGui::RootDock(pos, size);
            }

            if (mSettings.showImGuiTestView)
            {
                ImGui::ShowTestWindow(&mSettings.showImGuiTestView);
            }

            for (auto item : mPlugins)
            {
                item->onGUI();
            }
        }

        void render()
        {
            static const ImVec4 clear_color = ImColor(114, 144, 154);
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
            SDL_GL_SwapWindow(mWindow);
        }

        Settings                mSettings;
        bool                    mRunning;
        SDL_Window*             mWindow;
        SDL_GLContext           mGLContext;
        std::vector<IPlugin*>   mPlugins;
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
