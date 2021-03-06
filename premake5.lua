function externalSharedLib(location, dllname, env)
    local configs =
    {
        {
            filter="platforms:*32",
            arch="x86"
        },
        {
            filter="platforms:*64",
            arch="x64"
        }
    }
    for key,config in pairs(configs) do
        filter (config.filter)
            local command = '{COPY} "$(SolutionDir)..\\' .. location:gsub('/', '\\') .. '\\' .. config.arch .. '\\' .. dllname .. '.dll" "$(OutputPath)"'
            postbuildcommands { command }
    end
    
    filter {}
end

function externalStaticLib(location, libname)
    includedirs { location .. "/include" }

    filter "platforms:*32"
        libdirs { location .. "/lib/x86" }

    filter "platforms:*64"
        libdirs { location .. "/lib/x64" }

    filter {}

    links { "" .. libname }
end

function deployFiles(targetSubDir, files)
    postbuildcommands
    {
        "{MKDIR} " .. targetSubDir
    }
    for key,file in pairs(files) do
        postbuildcommands
        {
            "{COPY} " .. path.getabsolute(file) .. " " .. targetSubDir
        }
    end
end

function configure()
    configurations { "Debug", "Release" }
    platforms { "Win32", "x64" }
    
    filter "platforms:*32"
        system "Windows"
        architecture "x86"
        
    filter "platforms:*64"
        system "Windows"
        architecture "x86_64"

    filter {}
    includedirs
    {
        ".",
        "Contrib/imgui",
        "Contrib/imgui/examples/libs/gl3w",
        "Contrib/imgui/examples/sdl_opengl3_example",
    }
    symbols "On"
    flags { "FatalCompileWarnings" }
    warnings "Extra"
end

function StaticLib(name)
    project(name)
        kind "StaticLib"
        language "C++"
        configure()
end

function SharedLib(name)
    project(name)
        kind "SharedLib"
        language "C++"
        configure()
        defines { "EXPORT_" .. name:upper(), "SHARED_LIB" }
end

function ConsoleApplication(name)
    project(name)
        kind "ConsoleApp"
        language "C++"
        configure()
end

function WindowedApplication(name)
    project(name)
        kind "WindowedApp"
        language "C++"
        configure()
end

workspace "ManyEmu"
    location "build"
    startproject "ManyEmu"

    configure()

    filter "configurations:Debug"
        defines { "DEBUG" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter {}
        defines
        {
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_NONSTDC_NO_WARNINGS",
        }

StaticLib "ImGUI"
    files
    {
        "Contrib/imgui/*.h",
        "Contrib/imgui/*.cpp",
        "Contrib/imgui/examples/libs/gl3w/**.*",
        "Contrib/imgui/examples/imgui_impl_sdl.cpp",
        "Contrib/imgui/examples/imgui_impl_opengl3.cpp",
    }
    deployFiles("fonts",
        {
            "Contrib/imgui/misc/fonts/Cousine-Regular.ttf",
            "Contrib/imgui/misc/fonts/DroidSans.ttf",
            "Contrib/imgui/misc/fonts/Karla-Regular.ttf",
            "Contrib/imgui/misc/fonts/ProggyClean.ttf",
            "Contrib/imgui/misc/fonts/ProggyTiny.ttf",
            "Contrib/imgui/misc/fonts/Roboto-Medium.ttf",
        }
    )
    externalStaticLib("Contrib/SDL2", "SDL2")

StaticLib "yaml-cpp"
    files
    {
        "Contrib/yaml-cpp/include/**.h",
        "Contrib/yaml-cpp/src/**.h",
        "Contrib/yaml-cpp/src/**.cpp",
    }
    includedirs { "Contrib/yaml-cpp/include"}
    warnings "Default"
    disablewarnings { "4267" }

StaticLib "Core"
    files
    {
        "Core/**.h",
        "Core/**.cpp",
    }
    includedirs { "Contrib/yaml-cpp/include"}
    links { "yaml-cpp" }

StaticLib "Gameboy"
    files
    {
        "Gameboy/**.h",
        "Gameboy/**.cpp",
    }

StaticLib "NES"
    files
    {
        "NES/**.h",
        "NES/**.cpp",
    }

WindowedApplication "ManyEmu"
    files
    {
        "ManyEmu/**.h",
        "ManyEmu/**.cpp",
    }

    externalStaticLib("Contrib/SDL2", "SDL2")
    externalStaticLib("Contrib/SDL2", "SDL2main")
    externalStaticLib("Contrib/glew", "glew32s")
    externalSharedLib("Contrib/SDL2/lib", "SDL2")
    
    links
    {
        "ImGUI",
        "OpenGL32.lib",
        "GLU32.lib",
        "Core",
        "Gameboy",
        "NES",
    }
