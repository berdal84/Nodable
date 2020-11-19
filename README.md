<img src="https://github.com/berdal84/Nodable/blob/master/assets/nodable-logo-xs.png" />
   
<a href="https://github.com/berdal84/Nodable/actions?query=workflow%3Abuild" title="linux/windows x64">
<img src="https://github.com/berdal84/nodable/workflows/build/badge.svg" />
</a>

Nodable is node-able !
======================

Introduction:
-------------

The goal of **Nodable** is to **provide an original hybrid source code editor**, using both textual and nodal paradigm.

What does it mean ?

In Nodable, textual and nodal point of views are strongly linked, in both ways:

- A change to the source code will update the graph.
- A change to the graph will update the source code.

_Disclaimer: Nodable is a prototype, do not expect too much from it._

Here, Nodable in action:

![Demo GIF](https://www.dalle-cort.fr/wp-content/uploads/2018/01/2019_06_06_Nodable_0.4.1wip_Berenger_Dalle-Cort.gif)

How to try:
-----------

Download binaries from [home page](https://www.dalle-cort.fr/nodable-node-oriented-programming/) or browse Release section.

How to compile ? :
----------------

Requirements:
- A **C++17** compatible build system (tested with make/g++-10 and MSVC14.27.29110)
- Libraries `libsdl2-dev` and `libegl1-mesa-dev` (for linux only, win32 binaries are included)
- **CMake 3.14+**

Clone the Nodable repository (with submodules):

```
git clone https://github.com/berdal84/Nodable.git --recurse-submodules
```

Configure and run the build:

```
cd ./Nodable
cmake . -B build
cmake --build build --config Release [--target install]
```
*Optional `--target install` is to create a clean `./install/Release` directory with only necessary files to run the software.*

Nodable will be built into `./build/`

To run it:
```
cd build
./Nodable
```

Licence:
-------
**Nodable** is licensed under the GPL License, see LICENSE for more information.

Each submodule are licensed, browse */extern* folder.

Credits :
---------

**Nodable** is developped by @berdal84

Dependencies

- SDL2 : https://www.libsdl.org/
- GLFW3 : http://www.glfw.org/
- *Dear ImGui* developed by Omar Cornut: https://github.com/omarcornut/imgui
- IconFontCppHeaders by Juliette Faucaut: https://github.com/juliettef/IconFontCppHeaders
- ImGuiColorTextEdit by BalazsJako : https://github.com/BalazsJako/ImGuiColorTextEdit
- mirror by Grouflon : https://github.com/grouflon/mirror
- ImGui FileBrowser by AirGuanZ: https://github.com/AirGuanZ/imgui-filebrowser
- LodePNG by Lode Vandevenne
