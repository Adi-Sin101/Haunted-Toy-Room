# 01 — Environment Setup

This chapter explains how the project is wired together: which libraries it uses, where they live,
how Visual Studio finds them, and how to build and run the program on any Windows machine.

## 1. Toolchain

| Item | Version / value |
|---|---|
| IDE | Visual Studio 2026 (v18), "Desktop development with C++" workload |
| Platform toolset | `v145` |
| Windows SDK | `10.0.26100.0` |
| Language standard | C++20 (`/std:c++20`) |
| Target | `x64` only (the bundled `glfw3.lib` is a 64-bit library) |
| OpenGL | 3.3 Core Profile |

## 2. Libraries (all bundled in `Libraries/`)

| Library | Files | Purpose |
|---|---|---|
| **GLFW** | `include/GLFW/glfw3.h`, `lib/glfw3.lib` | Creates the window and the OpenGL context, reads keyboard/mouse input, provides a timer. |
| **GLAD** | `include/glad/glad.h`, `include/KHR/khrplatform.h`, `src/gl/glad.c` | OpenGL is a *specification*; the functions live inside the graphics driver. GLAD asks the driver for the address of every GL 3.3 core function at run time (`gladLoadGL()`). Generated for `gl=3.3`, profile `core`. |
| **GLM** | `include/glm/**` | Header-only math library. We use its `vec`/`mat` **types** and matrix multiplication/inverse. The actual transformation matrices (translate, rotate, scale, shear, reflect, lookAt, perspective) are written by hand in `src/math/Transform3D.cpp` so every entry is visible. |
| **opengl32.lib** | Windows SDK | Windows' OpenGL entry DLL; GLFW needs it to create the WGL context. |

No other library is used. Images are read by our own BMP loader and textures are generated procedurally.

## 3. How Visual Studio finds the libraries

Everything in `HauntedToyRoom.vcxproj` is **relative to the project folder** (`$(ProjectDir)`), so the
project builds on any computer. The original template had absolute paths such as
`C:\Users\Nihal\source\repos\Project1\Libraries\include`, which break on every other machine — that was
the main "linking" fix.

```xml
<IncludePath>$(ProjectDir)Libraries\include;$(ProjectDir)src;$(IncludePath)</IncludePath>
<LibraryPath>$(ProjectDir)Libraries\lib;$(LibraryPath)</LibraryPath>
<AdditionalDependencies>glfw3.lib;opengl32.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

* **Include path** → the compiler can resolve `#include <GLFW/glfw3.h>`, `<glad/glad.h>`, `<glm/glm.hpp>`
  and our own headers as `#include "gl/Shader.h"`.
* **Library path + additional dependencies** → the linker finds `glfw3.lib` and `opengl32.lib`.
* `glad.c` is compiled as part of the project (it is source code, not a prebuilt library).

### The debug-CRT detail

`glfw3.lib` was compiled with `/MDd` (debug C runtime). The Debug configuration uses the same runtime, so
it links cleanly. The Release configuration uses `/MD`, therefore it tells the linker to ignore
`MSVCRTD.lib` (`IgnoreSpecificDefaultLibraries`) so only the release runtime is linked.
`/ignore:4099` silences the harmless warning that `glfw3.pdb` (debug symbols for GLFW) is not shipped.

### Preprocessor definitions

| Define | Why |
|---|---|
| `GLFW_INCLUDE_NONE` | stops `GLFW/glfw3.h` from including the system `<GL/gl.h>`. GLAD must provide all GL declarations; if another GL header comes first GLAD stops with *"OpenGL header already included"*. With this define the include order of `glad.h` and `glfw3.h` no longer matters. |
| `GLM_FORCE_SILENT_WARNINGS` | keeps GLM's own headers quiet at warning level 4 |
| `_DEBUG` / `NDEBUG`, `_CONSOLE` | standard configuration defines (console window shows the program's messages) |

The project compiles with **warning level 4 and zero warnings** in both configurations.

## 4. Renaming the template

| Before | After |
|---|---|
| `Project1.slnx` | `HauntedToyRoom.slnx` |
| `Project1.vcxproj` (+ `.filters`) | `HauntedToyRoom.vcxproj` (+ `.filters`) |
| `RootNamespace` `Project1` | `HauntedToyRoom` |
| `Main.cpp`, `shaderClass.*`, `VAO/VBO/EBO.*`, `glad.c` in the root | moved into `src/` (see below) |
| `default.vert/.frag` in the root | `shaders/` |

The project GUID was kept so the solution file still references the same project.
Unused Win32 configurations were removed (there is no 32-bit GLFW library).

## 5. Folder layout

```
HauntedToyRoom.slnx            solution (open this in Visual Studio)
HauntedToyRoom.vcxproj         project: compiles every src/**/*.cpp automatically
Libraries/                     GLFW, GLAD headers, GLM, KHR
src/
  main.cpp                     entry point
  core/                        window + main loop, input
  gl/                          thin OpenGL wrappers: Shader, VAO, VBO, EBO, Texture, Framebuffer, glad.c
  math/                        hand-written transformation matrices, ray intersection
  geometry/                    Vertex, Mesh, primitive generators (vertices + indices)
  scene/                       Transform, SceneNode (hierarchy), MatrixStack, Material, Light, Camera
  characters/                  Woody, Jessie, Buzz (one humanoid builder), Bullseye, RC car
  world/                       room, props, day/night cycle, story director
  render/                      raster renderer, GPU ray tracer, textures (procedural + BMP)
shaders/                       GLSL shaders (copied next to the .exe after each build)
assets/textures/               BMP images (copied next to the .exe after each build)
tools/                         make_poster.py - generates the poster BMP
docs/                          this documentation
bin/<Config>/                  build output (ignored by git)
build/<Config>/                intermediate object files (ignored by git)
```

The project uses wildcards (`src\**\*.cpp`, `src\**\*.h`), so a new file placed under `src/` is compiled
automatically without editing the project file.

## 6. Build and run

**VS Code:** open the project folder, then press **F5** and select a Haunted Toy Room launch configuration. The workspace tasks build the matching x64 configuration first. Use **Ctrl+Shift+B** to build Debug.

**Visual Studio:** open `HauntedToyRoom.slnx`, pick `Debug | x64` or `Release | x64`, press **F5**.
The debugger's working directory is the output folder, where the post-build step has copied `shaders/`
and `assets/`.

**Command line:**

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    HauntedToyRoom.slnx /p:Configuration=Release /p:Platform=x64 /m
bin\Release\HauntedToyRoom.exe
```

The executable also searches for `shaders/` next to itself and in the parent folders, so it can be
started from the project root as well.

### Regenerating the poster image

`assets/textures/poster.bmp` is produced by `python tools/make_poster.py` (pure Python, no packages).
It is committed, so this is only needed if you change the script.

## 7. Program start-up sequence (what `main` does, in order)

1. `glfwInit()` — initialise GLFW.
2. `glfwWindowHint(...)` — request an OpenGL **3.3 Core** context (no deprecated fixed-function API).
3. `glfwCreateWindow(...)` + `glfwMakeContextCurrent(...)` — create the window and make its context current.
4. `gladLoadGL()` — load all OpenGL function pointers. **No `gl*` call is allowed before this.**
5. `glViewport(...)` — map normalized device coordinates (−1…1) to window pixels.
6. Create shaders, meshes, textures; enter the render loop:
   `poll input → update (delta time) → clear → draw → glfwSwapBuffers`.
7. Destroy GL objects, `glfwTerminate()`.
