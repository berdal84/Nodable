
Nodable Change log :
====================

Author: Bérenger Dalle-Cort, 2017-2025

### v1.0:
    - use now C++20
    - introduced ASTScope (represented in GraphView too)
    - removed every single line related to compilation and execution (it's out of scope now)
    - removed WIN32 and macOS platforms
    - added WEB platform (uses emscripten)
    - graph is now always abstract
    - interpreter (build, run, debug etc) is disabled by default and is considered out of scope, but can be enabled via the Developer>Experimental menu,
    - improved the Graph user interface to create_new graph more easily,
    - implemented an optimized StateMachine to simplify code (it avoids an OOP version of it! Thanks to Rémi ;)),
    - refactor the whole app (uses init/get/shutdown pattern for managers),
    - fix memory leaks,
    
### v0.9:
    - fix graph creation (ForLoopNode / WhileLoopNode)
    - add WhileLoopNode
    - use shared buffer for the Tokens (huge perf gain, up to x4)
    - fix the lost of suffixes on close parenthesis
    - remove any std::shared_ptr<Token> (from +18% up to +125% perfs gain on Nodlang::parse_token())
    - remove all regex (+25% performance on Nodlang::parse() )
    - fixed lot of crashes
    - add help panel
    - add FontManager
    - run gui tests on github actions    
    - use freetype instead of truetype to get sharp small fonts
    - architecture:
        - nodable / framework split
        - Simplifications (less getters/setters, less inheritance, more composition, less virtuals...)
    - add a shortcut system (rely on SDL2),
    - nodable events can be binded to shortcuts,
    - whole file editing is now default, user can enable/disable isolation mode to see selected text as a graph,
    - add screen overlay to help user,
    - hide long wires for variable references,
    - show variable name in input fields,
    - mem leak fixes,
    - add Compiler to run code in the VirtualMachine,
    - remove CodeBlockNode, keep only ScopeNode and interfaces,
    - add ForLoopNode,
    - add SlotBag,
    - add AppContext to get managers (avoid singleton pattern),
    - remove GraphTraversal.

### v0.8:
    - use now C++11 instead of C++17
    - macOS 10.9+ compatible (C++11 \o/)
    - use JetBrains Mono fonts
    - Multi instructions
    - Conditional structures (if/else only)
    - Execution (with optional step-by-step)
    - Experimental "clipboard auto-paste" (File -> enable autopaste, copy something from a text editor to see the graph)

### v0.7:
    - CI for Linux and Windows
	- refactor of Language and related classes (Dictionnary, TokenType, etc.)
	- Parser now check types.
	- Parser now parse function calls.
	- refactor in ComponentBag: added FunctionComponent (can invoke a function defined with a Language)
	- Assign operator is now a real node.

### v0.6:
	- use now mirror (by @Grouflon) as reflection framework.
	- Result node constraint to stay on visible rect after updating.
	- Build using CMake.
	- Parser: functions rewrote, parenthesis, detailed logs.
	- Language class.	

### v0.5:
	- fix(NodeView): Variable node drawing crash.
	- First draft version of an UNDO/REDO system.

### v0.4:
	- A brand new logo !
	- Nodable is now able to open existing files and to save them.
	- The UI has a tab system to switch between multiple loaded files.
	- Bug fixes

### v0.3:
	- New Node_Assign : '=' can be used to assign a value to a symbol (ex: a = 10)
	- Now Able to perform binary operations on symbols (ex: c = a + b).
	- Node_Context : is now used as a factory.
	- Node : each node can get its contexts with Entity::getParent()
	- Added a change log.
	- Added version number into the header file (NDBL_VERSION_MAJOR, NDBL_VERSION_MINOR, NDBL_VERSION)

### v0.2:
	- New Binary Operations : Node_Substract, Node_Multiply, Node_Divide
	- Lexer : nos supports operator precedence.

### v0.1:
	- Node_Add : to add two Node_Numbers
	- Lexer : first version able to evaluate additions.
