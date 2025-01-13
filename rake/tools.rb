require_relative 'base'
require_relative 'libs'

#---------------------------------------------------------------------------
$tools_core = new_target_from_base("tools_core", TargetType::OBJECTS)
$tools_core.sources |= FileList[
    "src/tools/core/reflection/qword.cpp",
    "src/tools/core/reflection/Type.cpp",
    "src/tools/core/reflection/TypeRegister.cpp",
    "src/tools/core/reflection/variant.cpp",
    #"src/tools/core/memory/pointers.cpp",
    "src/tools/core/EventManager.cpp",
    "src/tools/core/FileSystem.cpp",
    "src/tools/core/format.cpp",
    "src/tools/core/log.cpp",
    "src/tools/core/StateMachine.cpp",
    "src/tools/core/System.cpp",
    "src/tools/core/TaskManager.cpp"
]

if PLATFORM_DESKTOP
    $tools_core.link_library |= [$whereami]
end

#---------------------------------------------------------------------------
$tools_gui = new_target_from_base("tools_gui", TargetType::OBJECTS)
$tools_gui.sources |= FileList[
    "src/tools/gui/geometry/BezierCurveSegment2D.cpp",
    "src/tools/gui/geometry/BoxShape2D.cpp",
    "src/tools/gui/geometry/Rect.cpp",
    "src/tools/gui/geometry/SpatialNode.cpp",
    "src/tools/gui/geometry/TRSTransform2D.cpp",
    "src/tools/gui/Action.cpp",
    "src/tools/gui/ActionManager.cpp",
    "src/tools/gui/ActionManagerView.cpp",
    "src/tools/gui/App.cpp",
    "src/tools/gui/AppView.cpp",
    "src/tools/gui/Config.cpp",
    "src/tools/gui/FontManager.cpp",
    "src/tools/gui/ImGuiEx.cpp",
    "src/tools/gui/ViewState.cpp",
    "src/tools/gui/TextureManager.cpp",
]
$tools_gui.link_library |= [$lodepng, $imgui]

if PLATFORM_DESKTOP
    $tools_gui.link_library |= [$gl3w]
end

#---------------------------------------------------------------------------

app = new_target_from_base("tools-gui-example", TargetType::EXECUTABLE)
app.sources |= FileList[
    "src/tools/gui-example/AppExample.cpp",
    "src/tools/gui-example/AppExampleView.cpp",
    "src/tools/gui-example/main.cpp"
]
app.link_library |= [$tools_core, $tools_gui]

#---------------------------------------------------------------------------

tools_test = new_target_from_base("tools-test", TargetType::EXECUTABLE)
tools_test.sources |= FileList[
    "src/tools/core/Delegate.specs.cpp",
    "src/tools/core/string.specs.cpp",
    "src/tools/core/reflection/reflection.specs.cpp",
    "src/tools/gui/geometry/SpatialNode.specs.cpp",
    "src/tools/gui/geometry/Rect.specs.cpp"
]
tools_test.linker_flags |= [
    "-lgtest",
    "-lgtest_main"
]
tools_test.link_library |= [$tools_core, $tools_gui]

#---------------------------------------------------------------------------
desc "Build tools"
namespace :tools do

    desc "Clean tools"
    task :clean => ['core:clean', 'gui:clean', 'app:clean', 'test:clean']
    desc "Rebuild tools"
    task :rebuild => ['clean', 'build']
    desc "Build tools"

    task :build => [
        'core:build',
        'gui:build',
        'app:build',
        'test:build'
    ]
    task :test  => ['test:run']

    namespace :core do
        tasks_for_target( $tools_core )
    end

    task :gui
    namespace :gui do
        tasks_for_target( $tools_gui )
    end

    task :app
    namespace :app do
        tasks_for_target( app )
    end

    namespace :test do
        tasks_for_target( tools_test )
    end

end
#---------------------------------------------------------------------------