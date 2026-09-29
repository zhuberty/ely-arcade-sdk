-- ely-arcade-sdk -- premake/ely_sdk.lua
--
-- Shared premake helpers used by ely-arcade-platform and by every game repo
-- so the raylib / SDK / platform-specific build logic lives in ONE place.
--
-- Usage (from a premake5.lua that lives in a "build/" folder):
--
--     dofile("<path-to-sdk>/premake/ely_sdk.lua")
--     ely.prepare_dirs()
--     workspace "MyWorkspace" ... end
--     ely.raylib_project()
--     ely.sdk_project("<path-to-sdk>")
--     ely.app_project("my-game", "../src", "<path-to-sdk>")
--
-- All paths are relative to the calling premake5.lua (i.e. the build/ dir).

ely = ely or {}

ely.raylib_dir = "external/raylib-master"

-- ---------------------------------------------------------------------------
-- Command line options (same as the original raylib-quickstart)
-- ---------------------------------------------------------------------------

newoption
{
    trigger = "graphics",
    value = "OPENGL_VERSION",
    description = "version of OpenGL to build raylib against",
    allowed = {
        { "opengl11", "OpenGL 1.1"},
        { "opengl21", "OpenGL 2.1"},
        { "opengl33", "OpenGL 3.3"},
        { "opengl43", "OpenGL 4.3"},
        { "openges2", "OpenGL ES2"},
        { "openges3", "OpenGL ES3"},
        { "software", "OpenGL 1.1 Software Render"}
    },
    default = "opengl33"
}

newoption
{
    trigger = "backend",
    value = "BACKEND_TYPE",
    description = "Backend Platform to use",
    allowed = {
        { "glfw", "GLFW"},
        { "rgfw", "RGFW"},
        { "win32", "WIN32"},
    },
    default = "glfw"
}

newoption
{
    trigger = "wayland",
    value = "WAYLAND",
    description = "build for wayland",
    allowed = {
        { "off", "Off"},
        { "on", "On"}
    },
    default = "off"
}

-- ---------------------------------------------------------------------------
-- raylib download
-- ---------------------------------------------------------------------------

function ely.download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Download progress (" .. percent .. "%/100%)")
end

local function check_raylib()
    os.chdir("external")
    if(os.isdir("raylib-master") == false) then
        if(not os.isfile("raylib-master.zip")) then
            print("Raylib not found, downloading from github")
            local result_str, response_code = http.download("https://github.com/raysan5/raylib/archive/refs/heads/master.zip", "raylib-master.zip", {
                progress = ely.download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("raylib-master.zip", os.getcwd())
        os.remove("raylib-master.zip")
    end
    os.chdir("../")
end

-- Creates build_files/ and external/ next to the calling premake5.lua and
-- makes sure the raylib sources are present.
function ely.prepare_dirs()
    if (os.isdir('build_files') == false) then
        os.mkdir('build_files')
    end
    if (os.isdir('external') == false) then
        os.mkdir('external')
    end
    print("calling externals")
    check_raylib()
end

-- Standard workspace (configs, platforms, output dir). Call after prepare_dirs().
function ely.workspace(name)
    workspace (name)
        location "../"
        configurations { "Debug", "Release"}
        platforms { "x64", "x86", "ARM64"}

        defaultplatform ("x64")

        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"

        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"

        filter {"configurations:Release", "action:vs*"}
           linktimeoptimization "On"

        filter { "platforms:x64" }
            architecture "x86_64"

        filter { "platforms:ARM64" }
            architecture "ARM64"

        filter {}

        targetdir "bin/%{cfg.buildcfg}/"
end

-- ---------------------------------------------------------------------------
-- Platform defines / system libraries
-- ---------------------------------------------------------------------------

function ely.platform_defines()
    filter {"options:backend=glfw"}
        defines{"PLATFORM_DESKTOP"}

    filter {"options:backend=rgfw"}
        defines{"PLATFORM_DESKTOP_RGFW"}

    filter {"options:backend=win32"}
        defines{"PLATFORM_DESKTOP_WIN32"}

    filter {"options:graphics=opengl43"}
        defines{"GRAPHICS_API_OPENGL_43"}

    filter {"options:graphics=opengl33"}
        defines{"GRAPHICS_API_OPENGL_33"}

    filter {"options:graphics=opengl21"}
        defines{"GRAPHICS_API_OPENGL_21"}

    filter {"options:graphics=opengl11"}
        defines{"GRAPHICS_API_OPENGL_11"}

    filter {"options:graphics=openges3"}
        defines{"GRAPHICS_API_OPENGL_ES3"}

    filter {"options:graphics=openges2"}
        defines{"GRAPHICS_API_OPENGL_ES2"}

    filter {"options:graphics=software"}
        defines{"GRAPHICS_API_OPENGL_11_SOFTWARE"}

    filter {"system:macosx"}
        disablewarnings {"deprecated-declarations"}

    filter {"system:linux", "options:wayland=off"}
        defines {"_GLFW_X11"}

    filter {"system:linux", "options:wayland=on"}
        defines {"_GLFW_WAYLAND"}

    filter {}
end

-- System libraries an executable needs to link raylib.
function ely.link_system_libs()
    filter "system:linux"
        links {"pthread", "m", "dl", "rt"}

    filter {"system:linux", "options:wayland=off"}
        links {"X11"}

    filter {"system:linux", "options:wayland=on"}
        links {"wayland-client", "wayland-cursor", "wayland-egl", "xkbcommon"}

    filter "system:windows"
        defines{"_WIN32"}
        links {"winmm", "gdi32", "opengl32"}
        libdirs {"../bin/%{cfg.buildcfg}"}

    filter "system:macosx"
        links {"OpenGL.framework", "Cocoa.framework", "IOKit.framework",
               "CoreFoundation.framework", "CoreAudio.framework",
               "CoreVideo.framework", "AudioToolbox.framework"}

    filter {}
end


-- ---------------------------------------------------------------------------
-- Projects
-- ---------------------------------------------------------------------------

-- Static library containing raylib itself.
function ely.raylib_project()
    local raylib_dir = ely.raylib_dir

    project "raylib"
        kind "StaticLib"

        ely.platform_defines()

        location "build_files/"

        language "C"
        targetdir "../bin/%{cfg.buildcfg}"

        filter {"options:wayland=on"}
            defines {"GLFW_LINUX_ENABLE_WAYLAND=TRUE" }

        filter {"options:wayland=on", "system:linux"}
            prebuildcommands {
                "@echo Generating Wayland protocols...",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/wayland.xml ../" .. raylib_dir .. "/src/wayland-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-shell.xml ../" .. raylib_dir .. "/src/xdg-shell-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-decoration-unstable-v1.xml ../" .. raylib_dir .. "/src/xdg-decoration-unstable-v1-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/viewporter.xml ../" .. raylib_dir .. "/src/viewporter-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/relative-pointer-unstable-v1.xml ../" .. raylib_dir .. "/src/relative-pointer-unstable-v1-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/pointer-constraints-unstable-v1.xml ../" .. raylib_dir .. "/src/pointer-constraints-unstable-v1-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/fractional-scale-v1.xml ../" .. raylib_dir .. "/src/fractional-scale-v1-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-activation-v1.xml ../" .. raylib_dir .. "/src/xdg-activation-v1-client-protocol.h",
                "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/idle-inhibit-unstable-v1.xml ../" .. raylib_dir .. "/src/idle-inhibit-unstable-v1-client-protocol.h",
            }
        filter {}

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }
        filter{}

        includedirs {raylib_dir .. "/src", raylib_dir .. "/src/external/glfw/include" }
        vpaths
        {
            ["Header Files"] = { raylib_dir .. "/src/**.h"},
            ["Source Files/*"] = { raylib_dir .. "/src/**.c"},
        }
        files {raylib_dir .. "/src/*.h", raylib_dir .. "/src/*.c"}

        removefiles {raylib_dir .. "/src/rcore_*.c"}

        filter { "system:macosx", "files:" .. raylib_dir .. "/src/rglfw.c" }
            compileas "Objective-C"

        filter{}
end

-- The SDK itself, built as a static library that apps link against.
--   sdkDir : path to the ely-arcade-sdk checkout, relative to build/
function ely.sdk_project(sdkDir)
    project "ely-arcade-sdk"
        kind "StaticLib"
        location "build_files/"
        targetdir "../bin/%{cfg.buildcfg}"

        language "C++"
        cdialect "C17"
        cppdialect "C++17"

        files { sdkDir .. "/src/**.cpp", sdkDir .. "/include/**.h", sdkDir .. "/include/**.hpp" }
        includedirs { sdkDir .. "/include", ely.raylib_dir .. "/src" }

        flags { "ShadowedVariables" }
        ely.platform_defines()

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }
        filter{}
end

-- A console executable (game or menu) linked against raylib and the SDK.
--   name   : project / binary name
--   srcDir : the app's source dir, relative to build/
--   sdkDir : path to the ely-arcade-sdk checkout, relative to build/
function ely.app_project(name, srcDir, sdkDir)
    project (name)
        kind "ConsoleApp"
        location "build_files/"
        targetdir "../bin/%{cfg.buildcfg}"

        files { srcDir .. "/**.c", srcDir .. "/**.cpp", srcDir .. "/**.h", srcDir .. "/**.hpp" }
        includedirs { srcDir, sdkDir .. "/include", ely.raylib_dir .. "/src" }

        -- ely-arcade-sdk depends on raylib, so it must come first for static linking.
        links { "ely-arcade-sdk", "raylib" }

        cdialect "C17"
        cppdialect "C++17"

        flags { "ShadowedVariables" }
        ely.platform_defines()

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            dependson {"raylib"}
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }
        filter {}

        ely.link_system_libs()
end

