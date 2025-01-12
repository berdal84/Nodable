# Project  CMakeLists.txt
#========================

# include *.cmake scripts
include(scripts/cmake/log.cmake)
include(scripts/cmake/set-build-ref-variable.cmake)

# prerequisites:
cmake_policy(VERSION 3.14)
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum OS X deployment version") # must be declared BEFORE project
# GitHub Actions does not support v11

# project declaration
project("Nodable"
        VERSION "1.0"
        LANGUAGES CXX C)

# options/variables
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/out/app")      # ./out/ folder must not be changed, referenced in readme.md
set(NDBL_APP_NAME "Nodable")
set(NDBL_PACKAGE_DIR "${CMAKE_CURRENT_LIST_DIR}/out/package")  #     //
set(NDBL_CONFIGURED_DIR "${PROJECT_BINARY_DIR}/configured/ndbl")
set(NDBL_SKIP_TESTS OFF)
set(TOOLS_POOL_ENABLE OFF) # Keep OFF until refactor, PoolID<T> is too intrusive.

# set a build type to Release by default if not set:
if ( NOT DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release)
endif()

# Get architecture
if ( NOT ${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    ndbl_err("A 64bits architecture CPU is required to build this project")
endif ()

# avoid "unable to run shared libraries" under linux
include(CheckPIESupported)
check_pie_supported()

# specify the C++ standard
set(CMAKE_CXX_STANDARD            20)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_EXTENSIONS          OFF) # when OFF, uses lib++20/gnu++20 instead of c++20, see https://cmake.org/cmake/help/latest/prop_tgt/CXX_EXTENSIONS.html#prop_tgt:CXX_EXTENSIONS

# enable threads (we have some std::async in ndbl_app)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Defines TOOLS_DEBUG/NDBL_DEBUG when in Debug
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTOOLS_DEBUG -DNDBL_DEBUG")

if(WIN32)
    add_compile_definitions(NOMINMAX) # avoid min/max macros causing conflicts with min/max functions
endif ()

if (TOOLS_NO_POOL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTOOLS_NO_POOL")
endif ()

# disable char8_t when using CPP 20
if ( CMAKE_CXX_STANDARD EQUAL 20 )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-char8_t")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-char8_t")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-char8_t")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:char8_t-")
    else ()
        ndbl_err("The compiler you are using is not handled my this script, you have to manually add a case to is if/else in order to set a flag to disable char8_t")
    endif()
endif()

# add subdirectories
#-------------------

# Libraries: we only add the libraries that have a CMakeList, otherwise we append sources directly.

include(FetchContent)
FetchContent_Declare(
    cpptrace
    GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
    GIT_TAG        v0.5.4 # <HASH or TAG>
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cpptrace)

if (NOT NDBL_SKIP_TESTS)
    enable_testing()
    # https://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # we don't need mocks (for now) nor install
    option(BUILD_GMOCK OFF)
    option(INSTALL_GTEST OFF)
    add_subdirectory(libs/googletest EXCLUDE_FROM_ALL)
endif ()

set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(libs/google/benchmark EXCLUDE_FROM_ALL)
set(GLM_EXT_INCLUDED ON)
add_subdirectory(libs/glm EXCLUDE_FROM_ALL)
add_subdirectory(libs/freetype EXCLUDE_FROM_ALL)
set(NFD_PORTAL ON) # use portal instead of gtk, read: https://github.com/btzy/nativefiledialog-extended#using-xdg-desktop-portal-on-linux
add_subdirectory(libs/nativefiledialog-extended EXCLUDE_FROM_ALL) # target is nfd
add_subdirectory(libs/SDL EXCLUDE_FROM_ALL) # SDL2

# 1) Nodable
#===========

ndbl_log("Looking for libraries ...")

find_package(Threads REQUIRED)
if (NOT Threads_FOUND)
    ndbl_err("Threads not found")
endif ()

find_package(OpenGL REQUIRED)
if (NOT OpenGL_FOUND)
    ndbl_err("OpenGL not found")
endif ()

ndbl_log("Threads found: ${Threads_FOUND}")
ndbl_log("OpenGL found:  ${OpenGL_FOUND}")

# Includes for all (tools and nodable) targets
include_directories(
    src
    libs
    libs/Observe/include
    libs/SDL/include
    libs/gl3w
    libs/gl3w/GL
    libs/imgui
    libs/whereami/src
    libs/glm
    libs/IconFontCppHeaders
)


# 1.1) Framework Core
#--------------------

add_library(
    tools-core
    STATIC
    src/tools/core/Component.h
    src/tools/core/Event.h
    src/tools/core/EventManager.cpp
    src/tools/core/EventManager.h
    src/tools/core/FileSystem.cpp
    src/tools/core/FileSystem.h
    src/tools/core/Hash.h
    src/tools/core/StateMachine.cpp
    src/tools/core/StateMachine.h
    src/tools/core/System.cpp
    src/tools/core/System.h
    src/tools/core/TIdentifier.h
    src/tools/core/TaskManager.cpp
    src/tools/core/TaskManager.cpp
    src/tools/core/TaskManager.h
    src/tools/core/TaskManager.h
    src/tools/core/TryCatch.h
    src/tools/core/assertions.h
    src/tools/core/format.cpp
    src/tools/core/format.h
    src/tools/core/log.cpp
    src/tools/core/log.h
    src/tools/core/math.h
    src/tools/core/memory/Pool.h
    src/tools/core/memory/Pool.inl
    src/tools/core/memory/PoolManager.cpp
    src/tools/core/memory/PoolManager.h
    src/tools/core/memory/memory.h
    src/tools/core/memory/pointers.cpp
    src/tools/core/memory/pointers.h
    src/tools/core/reflection/Initializer.h
    src/tools/core/reflection/Invokable.h
    src/tools/core/reflection/Operator.h
    src/tools/core/reflection/Operator_t.h
    src/tools/core/reflection/Type.cpp
    src/tools/core/reflection/Type.h
    src/tools/core/reflection/TypeRegister.cpp
    src/tools/core/reflection/TypeRegister.h
    src/tools/core/reflection/enum.h
    src/tools/core/reflection/qword.cpp
    src/tools/core/reflection/qword.h
    src/tools/core/reflection/reflection
    src/tools/core/reflection/union.h
    src/tools/core/reflection/variant.cpp
    src/tools/core/reflection/variant.h
    src/tools/core/string.h
    src/tools/core/types.h
    src/tools/gui/Color.h
    # libraries
    libs/whereami/src/whereami.c
    libs/whereami/src/whereami.h
)

target_link_libraries(
    tools-core
    PUBLIC
    glm # math
    Threads::Threads
    cpptrace::cpptrace
)

add_executable(
    test-tools
    src/tools/core/UniqueList.specs.cpp
    src/tools/core/Variant.specs.cpp
    src/tools/core/UniqueVariantList.specs.cpp
    src/tools/core/Delegate.specs.cpp
    src/tools/core/reflection/reflection.specs.cpp
    src/tools/core/reflection/Type.specs.cpp
    src/tools/core/memory/Pool.specs.cpp
    src/tools/gui/geometry/Rect.specs.cpp
    src/tools/gui/geometry/SpatialNode.specs.cpp
)

target_link_libraries(test-tools PUBLIC gtest_main gtest tools-core tools-gui)
add_test(NAME test_tools COMMAND test-tools)

add_executable(bench-fw-core-string src/tools/core/string.bench.cpp)
target_link_libraries(bench-fw-core-string PUBLIC benchmark::benchmark tools-core)

if( TOOLS_POOL_ENABLE )
    add_executable(bench-fw-core-Pool src/tools/core/memory/Pool.bench.cpp
        NAMING.md)
    target_link_libraries(bench-fw-core-Pool PUBLIC benchmark::benchmark tools-core)
endif()

# 1.2) Framework GUI
#-------------------

add_library(
    tools-gui
    STATIC
    src/tools/gui/geometry/BoxShape2D.h
    src/tools/gui/geometry/BoxShape2D.cpp
    src/tools/gui/geometry/BezierCurveSegment2D.h
    src/tools/gui/geometry/BezierCurveSegment2D.cpp
    src/tools/gui/geometry/Rect.h
    src/tools/gui/geometry/Rect.cpp
    src/tools/gui/geometry/Space.h
    src/tools/gui/geometry/Vec2.h
    src/tools/gui/geometry/Vec4.h
    src/tools/gui/geometry/SpatialNode.h
    src/tools/gui/geometry/SpatialNode.cpp
    src/tools/gui/geometry/TRSTransform2D.h
    src/tools/gui/geometry/TRSTransform2D.cpp
    src/tools/gui/Action.cpp
    src/tools/gui/Action.h
    src/tools/gui/ActionManager.cpp
    src/tools/gui/ActionManager.h
    src/tools/gui/ActionManagerView.cpp
    src/tools/gui/ActionManagerView.h
    src/tools/gui/App.cpp
    src/tools/gui/App.h
    src/tools/gui/AppView.cpp
    src/tools/gui/AppView.h
    src/tools/gui/Config.cpp
    src/tools/gui/Config.h
    src/tools/gui/FontManager.cpp
    src/tools/gui/FontManager.h
    src/tools/gui/FontManagerConfig.h
    src/tools/gui/ImGuiEx.cpp
    src/tools/gui/ImGuiEx.h
    src/tools/gui/ImGuiExConfig.h
    src/tools/gui/Texture.h
    src/tools/gui/TextureManager.cpp
    src/tools/gui/TextureManager.h
    src/tools/gui/ViewState.cpp
    src/tools/gui/ViewState.h
    src/tools/gui/ImGuiTypeConvert.h
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/misc/freetype/imgui_freetype.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp # include backend as-is
    libs/imgui/backends/imgui_impl_sdl.cpp     #  //
    libs/ImGuiColorTextEdit/TextEditor.cpp
    libs/gl3w/GL/gl3w.c # Open GL Wrangler
    libs/gl3w/GL/gl3w.h
    libs/gl3w/GL/gl3w.gcda
    libs/gl3w/GL/gl3w.gcno
    libs/lodepng/lodepng.cpp
)

target_link_libraries(
    tools-gui
    PUBLIC
    ${CMAKE_DL_LIBS}
    OpenGL::GL
    SDL2-static SDL2main
    nfd # native file dialog - extended
    freetype # because: https://github.com/ocornut/imgui/tree/master/misc/freetype
    tools-core
)

# link APPLE specific libraries
IF (APPLE)
    ndbl_log("Looking for Apple specific libraries ...")

    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    if (NOT CORE_FOUNDATION_FRAMEWORK)
        ndbl_err("CoreFoundation not found")
    endif ()

    find_library(COCOA_FRAMEWORK Cocoa)
    if (NOT COCOA_FRAMEWORK)
        ndbl_err("Cocoa not found")
    endif ()

    ndbl_log("CORE_FOUNDATION_FRAMEWORK: ${CORE_FOUNDATION_FRAMEWORK}")
    ndbl_log("COCOA_FRAMEWORK:   ${COCOA_FRAMEWORK}")

    target_link_libraries(
        tools-gui
        PUBLIC
        ${CORE_FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
    )
ENDIF ()

target_compile_definitions(tools-gui PUBLIC IMGUI_USER_CONFIG="${CMAKE_CURRENT_LIST_DIR}/src/tools/gui/ImGuiExConfig.h") # Override imconfig.h
set_target_properties(tools-gui PROPERTIES POSITION_INDEPENDENT_CODE FALSE)       # required to run well on recent os (ex: ubuntu)
set_target_properties(tools-gui PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BUILD_PATH}") # fix working directory for visual studio

# define assets
set(TOOLS_ASSETS
    "assets/fonts/CenturyGothic.ttf"
    "assets/fonts/fa-solid-900.ttf"
)

# copy each file (will dirty the build when file changes)
foreach (ASSET ${TOOLS_ASSETS})
    configure_file("${ASSET}" "${CMAKE_BINARY_DIR}/${ASSET}" COPYONLY)
endforeach ()

# 1.3) Framework GUI - Example application
#---------------------------------------

# add executable
add_executable(
    tools-gui-example
    WIN32
    src/tools/gui-example/main.cpp
    src/tools/gui-example/AppExample.cpp
    src/tools/gui-example/AppExample.h
    src/tools/gui-example/AppExampleView.cpp
    src/tools/gui-example/AppExampleView.h
)
target_link_libraries(tools-gui-example PUBLIC tools-gui)
set_target_properties(tools-gui-example PROPERTIES POSITION_INDEPENDENT_CODE FALSE)       # required to run well on recent os (ex: ubuntu)
set_target_properties(tools-gui-example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BUILD_PATH}") # fix working directory for visual studio

# Installation :
# install( TARGETS tools-gui-example RUNTIME PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE DESTINATION .)

# 2.1) Nodable Core
#------------------

add_library(
    ndbl-core
    STATIC
    src/ndbl/core/ASTIf.cpp
    src/ndbl/core/ASTSlotLink.cpp
    src/ndbl/core/ASTForLoop.cpp
    src/ndbl/core/Graph.cpp
    src/ndbl/core/ASTUtils.cpp
    src/ndbl/core/ASTFunctionCall.cpp
    src/ndbl/core/ASTLiteral.cpp
    src/ndbl/core/ASTNode.cpp
    src/ndbl/core/ASTNodeFactory.cpp
    src/ndbl/core/ASTNodeProperty.cpp
    src/ndbl/core/ASTScope.cpp
    src/ndbl/core/ASTNodeSlot.cpp
    src/ndbl/core/ASTSwitchBehavior.cpp
    src/ndbl/core/ASTToken.cpp
    src/ndbl/core/ASTTokenRibbon.cpp
    src/ndbl/core/ASTVariable.cpp
    src/ndbl/core/ASTWhileLoop.cpp
    src/ndbl/core/language/Nodlang.cpp
    src/ndbl/core/NodableHeadless.cpp
    src/ndbl/core/NodableHeadless.h
)

target_link_libraries(
    ndbl-core
    PUBLIC
    tools-core
)

ndbl_log("Defining install ...")
set_target_properties(ndbl-core PROPERTIES OUTPUT_NAME "core")

ndbl_log("NDBL_SKIP_TESTS: ${NDBL_SKIP_TESTS}")

if (NDBL_SKIP_TESTS)
    return()
endif ()

add_executable(
    test-ndbl-core
    src/ndbl/core/Graph.specs.cpp
    src/ndbl/core/Graph.specs.cpp
    src/ndbl/core/ASTNodeSlot.specs.cpp
    src/ndbl/core/ASTToken.specs.cpp
    src/ndbl/core/language/Nodlang.basics.specs.cpp
    src/ndbl/core/language/Nodlang.tokenize.specs.cpp
    src/ndbl/core/language/Nodlang.parse_function_call.specs.cpp
    src/ndbl/core/language/Nodlang.parse_token.specs.cpp
    src/ndbl/core/language/Nodlang.parse_and_serialize.specs.cpp
)
target_link_libraries(test-ndbl-core PUBLIC gtest_main gtest ndbl-core)
add_test(NAME test_ndbl_core COMMAND test-ndbl-core)

# Benchmarks
add_executable(bench-ndbl-core-Nodlang src/ndbl/core/language/Nodlang.bench.cpp)
target_link_libraries(bench-ndbl-core-Nodlang PUBLIC benchmark::benchmark ndbl-core)

# 2.1) Nodable GUI
#------------------

# checks
ndbl_log("Checking ...")
if (NOT NDBL_CONFIGURED_DIR)
    ndbl_err("Variable NDBL_CONFIGURED_DIR must be set in main CMakeLists.txt")
endif ()

# defines
set(NDBL_APP_ASSETS_DIR assets)
set(NDBL_APP_ASSETS_ABSOLUTE_DIR "${CMAKE_CURRENT_LIST_DIR}/${NDBL_APP_ASSETS_DIR}")

# log variables
ndbl_log(" - NDBL_CONFIGURED_DIR:  ${NDBL_CONFIGURED_DIR}")
ndbl_log(" - NDBL_APP_ASSETS_DIR:  ${NDBL_APP_ASSETS_DIR}")
ndbl_log(" - NDBL_APP_ASSETS_ABSOLUTE_DIR: ${NDBL_APP_ASSETS_ABSOLUTE_DIR}")

# Configure files
configure_file(
    src/ndbl/gui/build_info.h.in
    ${NDBL_CONFIGURED_DIR}/gui/build_info.h
)

add_library(
    ndbl-gui
    STATIC
    src/ndbl/gui/Config.cpp
    src/ndbl/gui/Config.h
    src/ndbl/gui/GraphView.cpp
    src/ndbl/gui/ASTNodeViewContextualMenu.cpp
    src/ndbl/gui/History.cpp
    src/ndbl/gui/File.cpp
    src/ndbl/gui/FileView.cpp
    src/ndbl/gui/Nodable.cpp
    src/ndbl/gui/NodableView.cpp
    src/ndbl/gui/ASTNodeView.cpp
    src/ndbl/gui/PhysicsComponent.cpp
    src/ndbl/gui/ASTNodePropertyView.cpp
    src/ndbl/gui/ASTNodeSlotView.h
    src/ndbl/gui/ASTNodeSlotView.cpp
    src/ndbl/gui/Isolation.h
    src/ndbl/gui/ASTScopeView.cpp
)

target_link_libraries(
    ndbl-gui
    PUBLIC
    ndbl-core
    tools-gui
)

target_include_directories(
    ndbl-gui
    PUBLIC
    ${PROJECT_BINARY_DIR}/configured/ndbl/gui/
)

# define assets
set(ASSETS
    assets/examples/arithmetic.cpp
    assets/examples/for-loop.cpp
    assets/examples/if-else.cpp
    assets/examples/multi-instructions.cpp
    assets/fonts/JetBrainsMono-Bold.ttf
    assets/fonts/JetBrainsMono-Italic.ttf
    assets/fonts/JetBrainsMono-Medium.ttf
    assets/fonts/JetBrainsMono-Regular.ttf
    assets/images/nodable-logo-xs.png
)

# copy each file (will dirty the build when file changes)
foreach (ASSET ${ASSETS})
    configure_file("${ASSET}" "${CMAKE_BINARY_DIR}/${ASSET}" COPYONLY)
endforeach ()

ndbl_log("NDBL_SKIP_TESTS: ${NDBL_SKIP_TESTS}")

if (NDBL_SKIP_TESTS)
    return()
endif ()

add_executable(test-ndbl-gui src/ndbl/gui/Nodable.specs.cpp)
target_link_libraries(test-ndbl-gui PUBLIC gtest_main gtest ndbl-gui)

# GUI tests does not work on every machine (only MacOS in software on GitHub Actions)
if ($ENV{JETBRAINS_IDE})
    ndbl_log("JETBRAINS_IDE is defined:  Enable Nodable GUI tests (hardware rendering)")
    add_test(NAME test_ndbl_gui COMMAND test-ndbl-gui)
    add_definitions(-D NDBL_GUI_TEST_HUMAN_SPEED)
elseif (WIN32)
    ndbl_log("Windows detected: Skip Nodable GUI tests")
elseif (APPLE)
    ndbl_log("Apple detected: Enable Nodable GUI tests (software rendering)")
    add_test(NAME test_ndbl_gui COMMAND test-ndbl-gui)
elseif (UNIX) # Should be tested after APPLE
    ndbl_log("Linux detected: Skip Nodable GUI tests")
endif ()

# 2.1) Nodable App
#-----------------

add_executable(ndbl-app WIN32 src/ndbl/app/main.cpp)
target_link_libraries(ndbl-app PUBLIC ndbl-gui)
set_target_properties(ndbl-app PROPERTIES POSITION_INDEPENDENT_CODE FALSE)       # required to run well on recent os (ex: ubuntu)
set_target_properties(ndbl-app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BUILD_PATH}") # fix working directory for visual studio
set_target_properties(ndbl-app PROPERTIES OUTPUT_NAME "nodable")

set(README README.md)
set(LICENSE LICENSE)
configure_file("${README}" . COPYONLY)
configure_file("${LICENSE}" . COPYONLY)

# Installation :
install(TARGETS ndbl-app DESTINATION . RUNTIME PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)
install(DIRECTORY assets DESTINATION .)
install(FILES ${LICENSE} ${README} DESTINATION .)

# 3) Packaging:
#==============

include(scripts/cmake/cpack.cmake)
