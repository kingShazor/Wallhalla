# AGENTS.md

# Repository‑Quick‑Start

The repository contains a C++ shared library `recipe_picker` that is used by a Neovim Lua plugin. The library is compiled with CMake and offers a small interface exposed via FFI to Lua.

## Build

- **Standard build** – Generates `build/` with a Ninja generator.
  ```bash
  cmake --preset ninja   # (creates `build` directories for CMake & CTest)
  cmake --build build -j
  ```
- **Debug variant** – Uses clang++ in Debug mode.
  ```bash
  cmake --preset debug
  cmake --build build -j
  ```
- **Release with Zig** – For a minimal release using the Zig‑CPp frontend.
  ```bash
  cmake --preset release   # builds into `release_builds` with `zig‑cpp`
  cmake --build release_builds -j
  ```
- The shared library is placed in the matching binary directory:
  - `build/librecipe_picker.so` (Linux/Unix)
  - `build/librecipe_picker.dll` (Windows)

## Tests

After building, run the test suite with CTest or Ninja:
```bash
ctest --output-on-failure -j
# or
ninja -C build test
```
The repository includes two test executables: `fuzzy_sorter_test` and `picker_test`. If the test sources are missing, the build will fail; the repository now includes the needed files.

## Neovim integration

`lua/recipe-picker/native.lua` loads the library relative to the plugin directory. After building, Neovim will automatically find the library under `../../build/librecipe_picker.so` (or `.dll`). No additional steps are required.

## Common gotchas
- **Missing test files** – Earlier CMake configuration required `test/*_test.cpp`. Ensure these files exist or remove the test targets to avoid a build error.
- **Wrong preset** – `cmake --preset=base` may be confusing; use `ninja`, `debug`, or `release` as shown.
- **Linux vs Windows path** – Lua binding uses `../../build/...` relative path; the library must be in a top‑level `build` directory.
- **CMake export** – The preset turns on `CMAKE_EXPORT_COMPILE_COMMANDS`; deleting the preset or editing `CMakeLists` manually may unset this flag.
- **Test execution** – No separate lint or formatter step is defined. Running `ctest` after a successful build verifies everything.

## Summary for OpenCode
- Use `cmake --preset ninja` followed by `cmake --build build -j` to compile.
- Afterward, `ctest --output-on-failure -j` tests and `librecipe_picker.so` is ready for Neovim.
- If you encounter 'test files missing', double‑check the repository contains `test/fuzzy_sorter_test.cpp` and `test/picker_test.cpp`.

This file now reflects current build logic and prevents common mistakes.
