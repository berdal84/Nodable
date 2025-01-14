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

    target.asset_folder_path = "assets" # a single folder

    target.defines |= [
        "IMGUI_USER_CONFIG=\\\"tools/gui/ImGuiExConfig.h\\\"",
        "NDBL_APP_ASSETS_DIR=\\\"#{target.asset_folder_path}\\\"",
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
        target.linker_flags |= [
            "-sUSE_WEBGL2=1", # required for OpenGL ES 3.0 functionality.
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-fPIC", # Position Independent Code
			"-gsource-map",
			"--source-map-base=http://localhost:6931/bin/Web_Debug", # allow the web browser debugger to have c++ functions information
        ]

        target.compiler_flags |= [
            "-sUSE_FREETYPE=1",
            "-sUSE_SDL=2",
        ]

    elsif PLATFORM_DESKTOP
        target.includes |= [
            # ImGui includes SDL and freetype2 from their respective folder, we need to manually include them
            "/usr/include/freetype2",
            "/usr/include/SDL2",
        ]
        target.linker_flags |= [
            "-lnfd", # native file dialog (uses gtk)
            "`pkg-config --libs --static sdl2`",
            "`pkg-config --libs gtk+-3.0 freetype2 gl`",
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
    end

    target
end