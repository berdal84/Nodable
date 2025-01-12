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
        "-v"
    ]

    if BUILD_OS_LINUX or BUILD_OS_MACOS

        target.linker_flags |= [
            "-lGL", # opengl
            "-lfreetype -lpng -lz -lbrotlidec -lbz2",
            "-lSDL2 -lSDL2main",
            #"-lcpptrace -ldwarf -lz -lzstd -ldl", # https://github.com/jeremy-rifkin/cpptrace?tab=readme-ov-file#use-without-cmake
            "-lnfd `pkg-config --cflags --libs gtk+-3.0`",
        ]

        if BUILD_OS_MACOS

            target.linker_flags |= [
                "-lnfd -framework CoreFoundation -framework Cocoa"
            ]

            target.compiler_flags |= [
                "-mmacosx-version-min=#{MACOSX_VERSION_MIN}",
            ]
        end

    elsif BUILD_OS_MINGW

        target.linker_flags |= [
            "-lopengl32",
            "-lfreetype",
            "-lSDL2main -lSDL2",
            #"-lcpptrace -ldbghelp", # https://github.com/jeremy-rifkin/cpptrace?tab=readme-ov-file#use-without-cmake
            "-lnfd -lole32 -luuid -lshell32",
            "-luser32",
            "-lkernel32",
            "-lgdi32",
            "-limm32",
            "-lshell32",
            "-Xlinker /SUBSYSTEM:CONSOLE", # LINK : fatal error LNK1561: entry point must be defined (WinMain vs main, here we want to use main)
            "-Xlinker /NODEFAULTLIB"
        ]

        # see https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170#c-runtime-lib-files
        if BUILD_TYPE_DEBUG
            target.linker_flags |= [
                "-lmsvcrtd",
                "-lucrtd",
                "-lvcruntimed",
                "-llibcpmtd",
            ]
        else
            target.linker_flags |= [
                "-lmsvcrt",
                "-lucrt",
                "-lvcruntime",
                "-llibcpmt",
            ]
        end

        target.includes |= [
            "#{CMAKE_INSTALL_PREFIX_MINGW}\\include", # windows does not add to the path automatically
        ]

        target.linker_flags |= [
            "-L#{CMAKE_INSTALL_PREFIX_MINGW}\\lib", # windows does not add to the path automatically
        ]

        target.defines |= [
            "WIN32", # to have an MSVC-like macro 
            "NOMINMAX", # in WIN32, min and max are macros by default, it creates conflicts with std::min/std::max
        ] 

    end

    target
end