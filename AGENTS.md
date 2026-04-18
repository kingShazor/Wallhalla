# AGENTS.md

> A short reference for OpenCode agents to avoid common pitfalls in this repository.

## Build

- **Normal build** (debug or release with clang++)
  ```bash
  cmake --preset ninja   # (creates `build` directory with Ninja
  cmake --build build -j
  ```
- **Debug build (clag++ on Linux)**
  ```bash
  cmake --preset debug
  cmake --build build -j
  ```
- **Release build using Zig compiler**
  ```bash
  cmake --preset release   # uses zig-cpp
  cmake --build release_builds -j
  ```
- The compiled shared library is placed in the corresponding binary directory:
  - `build/librecipe_picker.so` (Linux/Unix)
  - `build/librecipe_picker.dll` (Windows)

## Tests

The CMake configuration enables CTest. After a build, run:
```bash
ctest --output-on-failure -j
```
or
```bash
ninja -C build test
```

## Neovim integration

`lua/recipe-picker/native.lua` resolves the library path relative to the plugin
directory, so after building the library it will be automatically found by
Neovim. No further action is required.

## Common gotchas
- **Using the wrong preset** – `cmake --preset=base` is the default; trying to `cmake .` will not create the build directory.
- **Missing `CMAKE_EXPORT_COMPILE_COMMANDS`** – the preset ensures it is on. If you edit CMakeLists manually, remember to re‑run the preset.
- **Linux vs Windows path** – the Lua bindings use `../../build/librecipe_picker.so` (or `.dll` on Windows). Ensure the build is in a `build` folder directly under the repo root.
- **Testing** – there is no separate lint or formatting step; running `ctest` is sufficient after building.
