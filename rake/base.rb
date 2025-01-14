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
            "-s WASM=1", 
            "-s FULL_ES2=1",
            "-s MIN_WEBGL_VERSION=2",
            "-s MAX_WEBGL_VERSION=2",
			"-gsource-map",
        ]

        files_to_preload = FileList["assets/**/*"]
        target.linker_flags |= files_to_preload.collect{ |file| "--preload-file #{file}" }
        target.compiler_flags |= [
            "-s USE_FREETYPE=1",
            "-s USE_SDL=2",
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