require_relative '_utils'

# Provide a base target
def new_target_from_base(name, type)

    target = new_empty_target(name, type)
    target.includes |= FileList[
        # internal
        "src",
        "src/ndbl",
        "src/tools",
        # external
        "libs",
        "libs/gl3w",
        "libs/gl3w/GL",
        "libs/glm",
        "libs/IconFontCppHeaders",
        "libs/imgui",
        "libs/imgui",
        "libs/whereami/src",
    ]

    target.defines |= [
        "IMGUI_USER_CONFIG=\\\"tools/gui/ImGuiExConfig.h\\\"",
        "NDBL_APP_NAME=\\\"nodable\\\"",
        "NDBL_BUILD_REF=\\\"local\\\"",
        "CPPTRACE_STATIC_DEFINE", #  error LNK2019: unresolved external symbol "__declspec(dllimport) public: void __cdecl cpptrace::stacktrace::print_with_snippets...
        "PLATFORM_#{PLATFORM.upcase}"
    ]

    target.cxx_flags |= [
        "-std=c++20",
        "-fno-char8_t",
    ]

    # ---- PLATFORM_XXX specific --------
    if PLATFORM_WEB
        target.compiler_flags |= [
            "-s USE_PTHREADS=1",
            "-s USE_FREETYPE=1",
            "-s USE_SDL=2",
            "-sNO_DISABLE_EXCEPTION_CATCHING",
			"-gsource-map",
        ]

        target.linker_flags |= [
            "-s PTHREAD_POOL_SIZE='navigator.hardwareConcurrency'",
            "-s EMBIND_STD_STRING_IS_UTF8=0",
            "-s ALLOW_MEMORY_GROWTH",
            "-s MIN_WEBGL_VERSION=2",
            "-s MAX_WEBGL_VERSION=2",
            "--preload-file assets/",
            "--emrun"
        ]

    elsif PLATFORM_DESKTOP
        target.includes |= [
            # ImGui includes SDL and freetype2 from their respective folder, we need to manually include them
            "/usr/include/freetype2",
            "/usr/include/SDL2",
        ]
        target.linker_flags |= [
            "-lnfd `pkg-config --libs gtk+-3.0`",
            "`pkg-config --libs --static sdl2`",
            "`pkg-config --libs freetype2 gl`",
        ] # NativeFileDialog
    end

    # ---- BUILD_TYPE_XXX specific --------
    if BUILD_TYPE_RELEASE
        target.compiler_flags |= [
            "-O2"
        ]
    elsif BUILD_TYPE_DEBUG
        target.compiler_flags |= [
            "-g", # generates symbols
            "-O0", # no optim
            "-Wfatal-errors",
            #"-pedantic"
        ]
        target.defines |= [
            "TOOLS_DEBUG",
            "NDBL_DEBUG"
        ]
    end

    target
end