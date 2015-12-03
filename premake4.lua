function adddll(location, dllname, env)
    local configs = { x32="x86", x64="x64" }
    for arch,config in pairs(configs) do
        configuration { arch }
            local command = 'xcopy /EQY "$(SolutionDir)..\\' .. location:gsub('/', '\\') .. '\\' .. config .. '\\' .. dllname .. '.dll" "$(OutputPath)"'
            postbuildcommands { command }
    end
    
    configuration {}
end

function addlibrary(location, libname)
    includedirs { location .. "/include" }

    configuration "x32"
        libdirs { location .. "/lib/x86" }

    configuration "x64"
        libdirs { location .. "/lib/x64" }

    configuration {}

    links { "" .. libname }
end

solution "ManyEmu"
    location "build"
    configurations { "Debug", "Release" }
    platforms { "x32", "x64" }

    configuration { "Debug", "x32" }
        targetdir "build/bin/x86/Debug"

    configuration { "Debug", "x64" }
        targetdir "build/bin/x64/Debug"

    configuration { "Release", "x32" }
        targetdir "build/bin/x86/Release"

    configuration { "Release", "x64" }
        targetdir "build/bin/x64/Release"

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }

    configuration "vs*"
        defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_WARNINGS" }

    configuration "vs2013"
        defines { "_USING_V110_SDK71_" }

project "ManyEmu"
    kind "ConsoleApp"
    language "C++"
    
    addlibrary("Contrib/SDL2", "SDL2")
    addlibrary("Contrib/glew", "glew32s")
    adddll("Contrib/SDL2/lib", "SDL2")
    
    includedirs { "." }

    files { "Core/**.h", "Core/**.cpp" }
    files { "NES/**.h", "NES/**.cpp" }
    files { "ManyEmu/**.h", "ManyEmu/**.cpp" }

