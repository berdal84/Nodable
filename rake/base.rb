require_relative '_utils'

# Provide a base target
def new_target_from_base(name, type)

    target = new_empty_target(name, type)
    target.includes |= FileList[
        "src",
        "src/ndbl",
        "src/tools",
        "libs",
        "libs/whereami/src",
        "libs/imgui",
        "libs/imgui",
        "libs/glm",
        "libs/gl3w/GL",
        "libs/gl3w",
        "libs/SDL/include",
        "libs/IconFontCppHeaders",
        "/usr/include/X11/mesa/GL",
        "#{BUILD_DIR}/include",
        "#{BUILD_DIR}/include/freetype2"
    ]

    target.asset_folder_path = "assets" # a single folder

    target.defines |= [
        "IMGUI_USER_CONFIG=\\\"tools/gui/ImGuiExConfig.h\\\"",
        "NDBL_APP_ASSETS_DIR=\\\"#{target.asset_folder_path}\\\"",
        "NDBL_APP_NAME=\\\"#{target.name}\\\"",
        "NDBL_BUILD_REF=\\\"local\\\"",
        "CPPTRACE_STATIC_DEFINE", #  error LNK2019: unresolved external symbol "__declspec(dllimport) public: void __cdecl cpptrace::stacktrace::print_with_snippets...
    ]

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

    target.cxx_flags |= [
        "--std=c++20",
        "-fno-char8_t"
    ]
        
    target.linker_flags |= [
        "-L#{BUILD_DIR}/lib",
        "-v", #verbose
        "-lGL", # opengl
        "-lfreetype -lpng -lz -lbrotlidec -lbz2",
        "-lSDL2 -lSDL2main",
        "-lcpptrace -ldwarf -lz -lzstd -ldl", # https://github.com/jeremy-rifkin/cpptrace?tab=readme-ov-file#use-without-cmake
        "-lnfd `pkg-config --cflags --libs gtk+-3.0`",
    ]

    target
end