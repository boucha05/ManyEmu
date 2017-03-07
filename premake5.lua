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
        "Contrib/imgui_extensions",
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

function Application(name)
    project(name)
        kind "ConsoleApp"
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
        flags { "Optimize" }

    filter {}
        defines
        {
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_NONSTDC_NO_WARNINGS",
        }

StaticLib "ImGUI"
    files
    {
        "Contrib/imgui_extensions/*.h",
        "Contrib/imgui_extensions/*.inl",
        "Contrib/imgui_extensions/*.cpp",
        "Contrib/imgui/*.h",
        "Contrib/imgui/*.cpp",
        "Contrib/imgui/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp",
        "Contrib/imgui/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.h",
        "Contrib/imgui/examples/libs/gl3w/**.*",
    }
    removefiles
    {
        "Contrib/imgui/imgui.cpp"
    }
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

Application "ManyEmu"
    files
    {
        "ManyEmu/**.h",
        "ManyEmu/**.cpp",
    }

    externalStaticLib("Contrib/SDL2", "SDL2")
    externalStaticLib("Contrib/glew", "glew32s")
    externalSharedLib("Contrib/SDL2/lib", "SDL2")
    
    links
    {
        "ImGUI",
        "Core",
        "Gameboy",
        "NES",
    }
