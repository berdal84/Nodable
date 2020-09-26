<img src="https://www.dalle-cort.fr/wp-content/uploads/2019/07/2019_08_04_Nodable_Logo_V2.jpg" width=600 />
                                                                                              
![CI status for windows (windows-server-2019)](https://github.com/berdal84/nodable/workflows/windows/badge.svg)
![CI status for linux (ubuntu-latest)](https://github.com/berdal84/nodable/workflows/linux/badge.svg)

Nodable is node-able !
======================

Introduction:
-------------

This software is **a node-able bidirectionnal expression editor**.

More precisely, it means **Text-to-Node and Node-to-Text seamless edition**.

To download a binary: go to this [article](https://www.dalle-cort.fr/nodable-node-oriented-programming/). By the way, if you're interested by the architecture, the language or the history of Nodable you'll find some documentation too.

You still don't understand what I'm doing? I hope this GIF will make this more understandable:

![Demo GIF](https://www.dalle-cort.fr/wp-content/uploads/2018/01/2019_06_06_Nodable_0.4.1wip_Berenger_Dalle-Cort.gif)


How to compile ? :
==================

Requirements:
- A **C++17** compatible build system (tested with make/g++-10 and MSVC14.27.29110)
- Libraries `libsdl2-dev` and `libegl1-mesa-dev` (for linux only, win32 binaries are included)
- **CMake 3.14+**

Open a command line from the Nodable base folder and type:

```
cmake -E make_directory ./build
cmake --build . --config Release [--target install]
```
Nodable will be built into `./build/Release/`

Optionnal `--target install` is to create a clean `./install/Release` directory with only necessary files to run the software.


Dependencies / Credits :
==============

- SDL2 : https://www.libsdl.org/
- GLFW3 : http://www.glfw.org/
- *Dear ImGui* developed by Omar Cornut: https://github.com/omarcornut/imgui
- IconFontCppHeaders by Juliette Faucaut: https://github.com/juliettef/IconFontCppHeaders
- ImGuiColorTextEdit by BalazsJako : https://github.com/BalazsJako/ImGuiColorTextEdit
- mirror by Grouflon : https://github.com/grouflon/mirror

Licence:
=========
**Nodable** is licensed under the GPL License, see LICENSE for more information.

Each submodule are licensed, browse */extern* folder.
