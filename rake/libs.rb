require_relative "base"
require_relative "_utils"
require_relative "_cmake"

task :libs => 'libs:build'
namespace :libs do

    task :build => [
        # compile .a/.lib first
        'freetype:install',
        'googletest:install',
        'nfd:install',
        'sdl:install',
        'cpptrace:install',
        # then .o        
        'whereami:build',
        'gl3w:build',
        'imgui:build',
        'lodepng:build',
        'text_editor:build',
    ]

    namespace :gl3w do
        $gl3w = new_target_from_base("gl3w", TargetType::OBJECTS)
        $gl3w.sources |= FileList[
            "libs/gl3w/GL/gl3w.c"
        ]
        tasks_for_target( $gl3w )
    end

    namespace :text_editor do
        $text_editor = new_target_from_base("text_editor", TargetType::OBJECTS)
        $text_editor.sources |= FileList[
            "libs/ImGuiColorTextEdit/TextEditor.cpp"
        ]
        tasks_for_target( $text_editor )
    end

    namespace :lodepng do
        $lodepng = new_target_from_base("lodepng", TargetType::OBJECTS)
        $lodepng.sources |= FileList[
            "libs/lodepng/lodepng.cpp"
        ]
        tasks_for_target( $lodepng )    
    end
    
    namespace :imgui do
        $imgui = new_target_from_base("imgui", TargetType::OBJECTS)
        $imgui.sources |= FileList[
           "libs/imgui/imgui.cpp",
           "libs/imgui/imgui_demo.cpp",
           "libs/imgui/imgui_draw.cpp",
           "libs/imgui/imgui_tables.cpp",
           "libs/imgui/imgui_widgets.cpp",
           "libs/imgui/misc/freetype/imgui_freetype.cpp",
           "libs/imgui/backends/imgui_impl_sdl.cpp",
           "libs/imgui/backends/imgui_impl_opengl3.cpp",
        ]
        tasks_for_target( $imgui )
    end

    namespace :whereami do
        $whereami = new_target_from_base("whereami", TargetType::OBJECTS)
        $whereami.sources |= FileList[
            "libs/whereami/src/whereami.c"
        ]
        tasks_for_target( $whereami )
    end

    namespace :nfd do
        nfd = CMakeTarget.new(name: "nfd", path: "libs/nativefiledialog-extended" )
        tasks_for_cmake_target( nfd )
    end

    namespace :googletest do
       googletest = CMakeTarget.new(name: "googletest", path: "libs/googletest" )
       tasks_for_cmake_target( googletest )
    end

    namespace :freetype do
       freetype = CMakeTarget.new(name: "freetype", path: "libs/freetype" )
       tasks_for_cmake_target( freetype )
    end

    namespace :sdl do
       sdl = CMakeTarget.new(name: "sdl", path: "libs/SDL" )
       tasks_for_cmake_target( sdl )
    end

    namespace :cpptrace do
       cpptrace = CMakeTarget.new(name: "cpptrace", path: "libs/cpptrace" )
       tasks_for_cmake_target( cpptrace )
    end

end # namespace libs