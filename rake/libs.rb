require_relative 'base'

# declare here the external libraries we need to build as OBJECTS

#---------------------------------------------------------------------------
$gl3w = new_target_from_base("gl3w", TargetType::OBJECTS)
$gl3w.sources |= FileList[
    "libs/gl3w/GL/gl3w.c"
]

$text_editor = new_target_from_base("text_editor", TargetType::OBJECTS)
$text_editor.sources |= FileList[
    "libs/ImGuiColorTextEdit/TextEditor.cpp"
]

$lodepng = new_target_from_base("lodepng", TargetType::OBJECTS)
$lodepng.sources |= FileList[
    "libs/lodepng/lodepng.cpp"
]

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

$whereami = new_target_from_base("whereami", TargetType::OBJECTS)
$whereami.sources |= FileList[
    "libs/whereami/src/whereami.c"
]
#---------------------------------------------------------------------------
namespace :libs do

    if PLATFORM_DESKTOP
        task :build => [
            'whereami:build',
            'gl3w:build'
        ]
    end

    task :build => [
        'text_editor:build',
        'lodepng:build',
        'imgui:build'
    ]

    namespace :gl3w do
        tasks_for_target( $gl3w )
    end

    namespace :text_editor do
        tasks_for_target( $text_editor )
    end

    namespace :lodepng do
        tasks_for_target( $lodepng )
    end

    namespace :imgui do
        tasks_for_target( $imgui )
    end

    namespace :whereami do
        tasks_for_target( $whereami )
    end
end