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
        "libs/freetype/include/",
        "libs/gl3w",
        "libs/gl3w/GL",
        "libs/glm",
        "libs/IconFontCppHeaders",
        "libs/imgui",
        "libs/imgui",
        "libs/SDL/include",
        "libs/whereami/src",
        "libs/cpptrace/include"
    ]

    target.asset_folder_path = "assets" # a single folder

    target.defines |= [
        "IMGUI_USER_CONFIG=\\\"tools/gui/ImGuiExConfig.h\\\"",
        "NDBL_APP_ASSETS_DIR=\\\"#{target.asset_folder_path}\\\"",
        "NDBL_APP_NAME=\\\"nodable\\\"",
        "NDBL_BUILD_REF=\\\"local\\\"",
        "CPPTRACE_STATIC_DEFINE", #  error LNK2019: unresolved external symbol "__declspec(dllimport) public: void __cdecl cpptrace::stacktrace::print_with_snippets...
        "TARGET_#{TARGET.upcase}"
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
        "-lcpptrace -ldwarf -lz -lzstd -ldl", # CPPTrace, see https://github.com/jeremy-rifkin/cpptrace?tab=readme-ov-file#use-without-cmake
        "`pkg-config --libs gl`", #OpenGL           
        "`pkg-config --cflags --libs gtk+-3.0`", #NativeFileDialog  deps  
        "`pkg-config --libs freetype2`",
        "`pkg-config --libs --static sdl2`",
    ]

    if TARGET == "desktop"
       target.linker_flags |= [
            "-lnfd"
       ] # NativeFileDialog
    end

    if BUILD_TYPE_RELEASE
       target.linker_flags |= [
            "-v"
       ] # NativeFileDialog
    end

    target
end