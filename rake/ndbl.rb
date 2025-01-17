require_relative 'base'
require_relative 'tools'

#---------------------------------------------------------------------------
ndbl_core = new_target_from_base("ndbl_core", TargetType::OBJECTS)
ndbl_core.sources |= FileList[
    "src/ndbl/core/language/Nodlang.cpp",
    "src/ndbl/core/ASTForLoop.cpp",
    "src/ndbl/core/ASTFunctionCall.cpp",
    "src/ndbl/core/ASTIf.cpp",
    "src/ndbl/core/ASTLiteral.cpp",
    "src/ndbl/core/ASTNode.cpp",
    "src/ndbl/core/ASTNodeProperty.cpp",
    "src/ndbl/core/ASTNodeSlot.cpp",
    "src/ndbl/core/ASTScope.cpp",
    "src/ndbl/core/ASTSlotLink.cpp",
    "src/ndbl/core/ASTSwitchBehavior.cpp",
    "src/ndbl/core/ASTToken.cpp",
    "src/ndbl/core/ASTTokenRibbon.cpp",
    "src/ndbl/core/ASTUtils.cpp",
    "src/ndbl/core/ASTVariable.cpp",
    "src/ndbl/core/ASTWhileLoop.cpp",
    "src/ndbl/core/Graph.cpp",    
    "src/ndbl/core/NodableHeadless.cpp",
]

#---------------------------------------------------------------------------
ndbl_gui = new_target_from_base("ndbl_gui", TargetType::OBJECTS)
ndbl_gui.sources |= FileList[
    "src/ndbl/gui/ASTNodeSlotView.cpp", 
    "src/ndbl/gui/ASTNodeView.cpp", 
    "src/ndbl/gui/ASTNodeViewContextualMenu.cpp",
    "src/ndbl/gui/ASTNodePropertyView.cpp",  
    "src/ndbl/gui/ASTScopeView.cpp", 
    "src/ndbl/gui/Config.cpp",
    "src/ndbl/gui/File.cpp",
    "src/ndbl/gui/FileView.cpp",
    "src/ndbl/gui/GraphView.cpp",
    "src/ndbl/gui/History.cpp",
    "src/ndbl/gui/Nodable.cpp",
    "src/ndbl/gui/NodableView.cpp",
    "src/ndbl/gui/PhysicsComponent.cpp",       
]
#---------------------------------------------------------------------------
ndbl_app = new_target_from_base("nodable", TargetType::EXECUTABLE)
ndbl_app.sources |= FileList[
    "src/ndbl/app/main.cpp",
]

ndbl_assets = FileList[
    # Examples
    "examples/arithmetic.cpp",
    "examples/for-loop.cpp",
    "examples/if-else.cpp",
    "examples/multi-instructions.cpp",
    # Fonts
    "fonts/CenturyGothic.ttf",
    "fonts/fa-solid-900.ttf",
    "fonts/JetBrainsMono-*.ttf", # 4 variants
    # Images
    "images/nodable-logo-xs.png",
]

if PLATFORM_WEB

    # Preload assets (they will be compiled in a binary .data)
    compiler_flags |= ndbl_assets.map{|pattern| "--preload-file #{pattern.split(":")[0]}" }

    # Provide headers for deployment (note: requires https when deployed).
    ndbl_app.assets = [
        "http/.htaccess:.htaccess"
    ]

elsif  PLATFORM_DESKTOP
    ndbl_app.assets = ndbl_assets
end

ndbl_app.link_library |= [
    $tools_gui,
    $tools_core,
    $text_editor,
    ndbl_core,
    ndbl_gui
]
#---------------------------------------------------------------------------
ndbl_test = new_target_from_base("ndbl-specs", TargetType::EXECUTABLE)
ndbl_test.sources |= FileList[
    "src/ndbl/core/**/*.specs.cpp"
]

ndbl_test.linker_flags |= [
    "-lgtest",
    "-lgtest_main"
]

ndbl_test.link_library |= [
    $tools_core,
    $tools_gui,
    ndbl_core
]

# On GitHub actions, I couldn't run this test with Linux yet.
if ENV["NDBL_SKIP_GUI_TEST"]
    puts "NDBL_SKIP_GUI_TEST is ON, skip GUI specs."
else
    ndbl_test.assets |= ndbl_app.assets;

    ndbl_test.sources |= [
        "src/ndbl/gui/Nodable.specs.cpp"
    ]
    ndbl_test.link_library |= [
        ndbl_gui,
        $text_editor
    ]
end

#---------------------------------------------------------------------------
task :ndbl => 'ndbl:build'
namespace :ndbl do

    task :clean => ['core:clean', 'gui:clean', 'app:clean']
    task :rebuild => ['clean', 'build']
    task :build => ['core:build', 'gui:build', 'app:build']

    if !PLATFORM_WEB
        task :clean => ['test:clean']
        task :build => ['test:build']
        task :test  => ['test:run']
    end

    namespace :core do
        tasks_for_target( ndbl_core )
    end
    
    namespace :gui do        
        tasks_for_target( ndbl_gui )
    end

    namespace :app do
        tasks_for_target( ndbl_app )
    end

    namespace :test do
        tasks_for_target( ndbl_test )
    end
end
#---------------------------------------------------------------------------