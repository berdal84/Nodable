[Home](./README.md) ->  Build

# How to build?

This document explains how to build nodable from sources, if you just want to try it you can skip these steps and browse [https://nodable.42borgata.com](https://nodable.42borgata.com).

## Architecture

Nodable sources are split in two folders under [./src](./src/README.md):
- [./src/ndbl](src/ndbl/README.md) project.
- [./src/tools](src/tools/README.md) project.

They both rely on external [libraries](./libs/README.md).

## Prerequisites:

### System
- Ubuntu 22.04+

### Software
- clang (C++20 compatible)
- Ruby 3+
- CMake 3.14+ (required to build some libs)

## Build

### Clone the source code

Run the following command:
```console
git clone --branch v1.0 https://github.com/berdal84/nodable.git --recurse-submodules
```

Few details about the commands above:

- `--recurse-submodules` is important when cloning since *Nodable* needs other git repositories to be built.
- `--branch v<major>.<minor>.<patch>` is to target a specific tag, it is recommended to get a stable version. You can try a more recent if you wish. Browse [tags list](https://github.com/berdal84/nodable/tags).

### Install and Build

Run the following commands:

```console
cd nodable
rake install
rake build
```

_Note: The default platform is "desktop", but you can build for the "web" by exporting `PLATFORM=web` to your environment or adding it as command line argument._

Build output should be available in `build-desktop-release/bin`, simply run `./nodable` from this folder

## Run

Once built is done, the simplest way to run nodable is:

```console
rake run
```

## Dev

During development, you can generate a .clangd file to get static syntax analysis.
To generate the file, run:

```console
rake ndbl:app:clangd
```

This file is read by clangd to perform its analysis. Many IDEs and text editors are compatible with it.
