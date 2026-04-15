# Wave Tracer

Wave Tracer is a C++ real-time editor built with GLFW, ImGui, OpenGL, and a custom engine.

## Repository

- `engine/` - engine library source
- `editor/` - editor application source
- `libs/` - bundled dependencies

Third-party libraries are expected under `libs/`. If this repo uses Git submodules, run:

```bash
git submodule update --init --recursive
```

## Requirements

- CMake 3.13 or newer
- C++17 compiler
- OpenGL development headers and libraries
- Git

## Build

### Windows

```powershell
cd path\to\wave_tracer
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Debug
./editor/Debug/wave_tracer.exe
```

### Linux

```bash
cd ~/Desktop/projects/wave_tracer
mkdir -p build
cd build
cmake ..
cmake --build . --config Debug
./editor/Debug/wave_tracer
```

### macOS

```bash
cd ~/Desktop/projects/wave_tracer
mkdir -p build
cd build
cmake ..
cmake --build . --config Debug
./editor/Debug/wave_tracer
```

## Notes

- Use the `build/` directory for generated files.
- If the build fails due to missing libraries, confirm the `libs/` folder contains GLFW, ImGui, and GLM.
- On Windows, keep the CMake generator architecture consistent with your compiler.
