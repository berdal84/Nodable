namespace :ndbl do
    core = new_project("ndbl_core")
    core[:sources] |= FileList[
        "src/ndbl/core/language/Nodlang.cpp",
        "src/ndbl/core/language/Nodlang_biology.cpp",
        "src/ndbl/core/language/Nodlang_math.cpp",
        "src/ndbl/core/Code.cpp",
        "src/ndbl/core/Compiler.cpp",
        "src/ndbl/core/ComponentFactory.cpp",
        "src/ndbl/core/DirectedEdge.cpp",
        "src/ndbl/core/ForLoopNode.cpp",
        "src/ndbl/core/FunctionNode.cpp",
        "src/ndbl/core/Graph.cpp",
        "src/ndbl/core/IfNode.cpp",
        "src/ndbl/core/Instruction.cpp",
        "src/ndbl/core/Interpreter.cpp",
        "src/ndbl/core/LiteralNode.cpp",
        "src/ndbl/core/NodableHeadless.cpp",
        "src/ndbl/core/Node.cpp",
        "src/ndbl/core/NodeComponent.cpp",
        "src/ndbl/core/NodeFactory.cpp",
        "src/ndbl/core/Property.cpp",
        "src/ndbl/core/PropertyBag.cpp",
        "src/ndbl/core/Scope.cpp",
        "src/ndbl/core/Slot.cpp",
        "src/ndbl/core/SwitchBehavior.cpp",
        "src/ndbl/core/Token.cpp",
        #"src/ndbl/core/Token.specs.cpp",
        "src/ndbl/core/TokenRibbon.cpp",
        "src/ndbl/core/Utils.cpp",
        "src/ndbl/core/VariableNode.cpp",
        "src/ndbl/core/WhileLoopNode.cpp",
    ]

    gui = new_project("ndbl_gui")
    gui[:sources] |= FileList[
        "src/ndbl/gui/CreateNodeCtxMenu.cpp",
        "src/ndbl/gui/GraphView.cpp",
        "src/ndbl/gui/Nodable.cpp",
        #"src/ndbl/gui/benchmark.cpp",
        "src/ndbl/gui/Config.cpp",
        "src/ndbl/gui/File.cpp",
        "src/ndbl/gui/FileView.cpp",
        "src/ndbl/gui/History.cpp",
        "src/ndbl/gui/NodableView.cpp",
        "src/ndbl/gui/NodeView.cpp",
        "src/ndbl/gui/Physics.cpp",
        "src/ndbl/gui/PropertyView.cpp",
        "src/ndbl/gui/ScopeView.cpp",
        "src/ndbl/gui/SlotView.cpp",
    ]

    app = new_project("nodable")
    app[:sources] |= FileList[
        "src/ndbl/app/main.cpp",
    ]

    app[:sources] |= $tools_core[:sources]
    app[:sources] |= $tools_gui[:sources]
    app[:sources] |= core[:sources]
    app[:sources] |= gui[:sources]
    app[:sources] |= $gl3w[:sources]
    app[:sources] |= $lodepng[:sources]
    app[:sources] |= $imgui[:sources]
    app[:sources] |= $texteditor[:sources]

    app[:defines].merge!({
        "NDBL_APP_NAME":       "nodable",
        "NDBL_APP_ASSETS_DIR": "assets",
        "NDBL_BUILD_REF":      "local"
    })

    namespace :core do
        declare_project_tasks( core )
    end

    namespace :gui do
        declare_project_tasks( gui )
    end

    namespace :app do
        declare_project_tasks( app )
    end

    task :build => ['ndbl:core:link', 'ndbl:gui:link', 'ndbl:app:link']
end