# Haunted Toy Room

A computer graphics lab project in C++ and OpenGL 3.3. At night a child's toy room comes alive: Woody,
Jessie, Buzz, Bullseye and an RC car wander around, the desk lamp flickers and looks around, a beach ball
rolls by itself and a ghost drifts through the room. When morning comes, every toy walks back to its place
and stands still again.

Every toy is built from hand-written primitives (plane, cube, sphere, cylinder, cone). The project also
implements its own transformations, scene hierarchy, Phong illumination, Flat/Gouraud/Phong/Blinn shading,
procedural textures, a BMP loader and a real-time GPU ray tracer. The only libraries are GLFW, GLAD and
GLM.

## Build and run

Requirements: Windows, Visual Studio 2026 with the C++ desktop workload, and a GPU that supports OpenGL 3.3.

1. Open `HauntedToyRoom.slnx`.
2. Choose `Release | x64` (or `Debug | x64`).
3. Press **F5**.

All library paths are relative to the project folder, so no setup is needed. See
[docs/01-environment-setup.md](docs/01-environment-setup.md).

## First things to try

| Keys | What happens |
|---|---|
| **1–5** | select Woody, Jessie, Bullseye, Buzz, RC car (6 ball, 7 lamp, 8 ghost, or left-click any object) |
| **W/S/A/D**, Shift, SPACE | drive the selected toy, run, stop |
| **R** | Jessie mounts or dismounts Bullseye (walk her close to him first) |
| **Q/E**, **L** | Buzz flies and fires his laser; L also toggles the car's headlights |
| **F**, scroll, right-drag | orbit and zoom in on the selected object |
| **C** | camera mode: Free, Orbit, Follow |
| **Tab**, **T**, J/L U/O I/K | edit mode: translate, rotate, scale or shear any object |
| **F1 / F2 / F3** | wireframe, shading model, textures |
| **F4** | real-time ray tracing (shadows, reflections, transparency) |
| **[ ]  , .  P  N** | time speed, scrub time, pause, story on/off |
| **H** | full help |

Full key list: [docs/13-controls.md](docs/13-controls.md).

## Documentation

[docs/00-index.md](docs/00-index.md) lists every chapter: pipeline basics, the vertex and index tables of
each primitive, transformation matrices, the camera, the scene graph and mounting, the characters,
illumination, shading, textures, animation, ray tracing and the code architecture.

## Project layout

```
src/        C++ sources (core, gl, math, geometry, scene, characters, world, render, app)
shaders/    GLSL shaders (lighting.glsl is shared by all of them)
assets/     BMP textures
tools/      asset generator scripts
docs/       documentation
Libraries/  GLFW, GLAD, GLM
```
