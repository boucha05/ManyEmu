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
    includedirs { "." }
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

StaticLib "Core"
    files
    {
        "Core/**.h",
        "Core/**.cpp",
    }
    
    includedirs
    {
        "Contrib/SDL2/include",
    }

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
        "Core",
        "Gameboy",
        "NES",
    }
