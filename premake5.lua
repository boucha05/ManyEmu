function adddll(location, dllname, env)
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

function addlibrary(location, libname)
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
        defines { "EXPORT_" .. name:upper(), "EMU_MODULE" }
end

function Application(name)
    project "ManyEmu"
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
        flags { "Symbols" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        flags { "Symbols", "Optimize" }

    filter {}
        defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_WARNINGS", "_USING_V110_SDK71_" }

StaticLib "Core"
    files { "Core/**.h", "Core/**.cpp" }

StaticLib "Gameboy"
    files { "Gameboy/**.h", "Gameboy/**.cpp" }

StaticLib "NES"
    files { "NES/**.h", "NES/**.cpp" }

SharedLib "Emulator"
    files { "Emulator/**.h", "Emulator/**.cpp" }

Application "ManyEmu"    
    files { "ManyEmu/**.h", "ManyEmu/**.cpp" }
    addlibrary("Contrib/SDL2", "SDL2")
    addlibrary("Contrib/glew", "glew32s")
    adddll("Contrib/SDL2/lib", "SDL2")
    links { "Core", "Gameboy", "NES", "Emulator" }
