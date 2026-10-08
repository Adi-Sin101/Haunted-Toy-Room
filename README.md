# Haunted Toy Room

A computer graphics lab project in C++ and OpenGL 3.3. Penny's existing arrival through the garden and
house remains the prologue. The StoryDirector now owns an explicit gameplay state machine for the new
sequence: entrance puzzle, toy rescue, Buzz rescue, final escape, and win ending. Stage 1 has three physical
hallway clues and a keypad; the rescue, chase, and ending interactions are next-phase work.

Every toy is built from hand-written primitives (plane, cube, sphere, cylinder, cone). The project also
implements its own transformations, scene hierarchy, Phong illumination, Flat/Gouraud/Phong/Blinn shading,
procedural textures, a BMP loader and a real-time GPU ray tracer. The only libraries are GLFW, GLAD and
GLM.

## Build and run in VS Code

Requirements: Windows, Visual Studio 2026 or its Build Tools with the **Desktop development with C++** workload and **Windows 11 SDK 10.0.26100**, the VS Code **C/C++** extension, and a GPU that supports OpenGL 3.3. VS Code supplies the editor and debugger integration; the MSVC compiler and MSBuild come from Visual Studio/Build Tools.

1. Open this project folder in VS Code.
2. Press **F5** and choose **Run Haunted Toy Room (Debug)**. VS Code builds the x64 Debug configuration and starts the app with the correct working directory. Choose the Release launch configuration to run Release instead.
3. Use **Ctrl+Shift+B** to build Debug, or select **Build Haunted Toy Room (Release)** from **Terminal → Run Build Task**.

The build task finds MSBuild through Visual Studio's `vswhere` tool, or uses MSBuild already on `PATH`. Library paths are relative to the project folder. See [docs/01-environment-setup.md](docs/01-environment-setup.md) for environment details.

You can also open `HauntedToyRoom.slnx` in Visual Studio and press **F5**.

## First things to try

Lighting, surface shading, ray tracing and textures all start **on**.
Click the small **Settings** button at the top right, or press the **backtick (`)** key, to toggle each
feature independently. **G** hides the button, menu and the rest of the interface; press G again to show
them. All rendering implementations remain available. With lighting on and shading off, basic light
colour and distance attenuation remain, while normal-based shading, highlights and shadows are skipped.

Use **N** to switch between manual control and gameplay progression, **Shift+N** to replay Penny's arrival
and reset progression, and **P** to pause the world clock. **Y** skips Penny's arrival and starts Stage 1.
During Stage 1 Penny is automatically selected and controlled with **W/S** to move, **A/D** to turn and
**Space** to stop. Walk up to each hallway clue and press **Enter** to inspect it. At the keypad press
**Enter**, type the digits, then press **Enter** to submit; **Backspace** edits and **Escape** cancels.
Other character selection becomes available again after the puzzle is solved.
The interface is one compact corner panel: **H** expands its guide; **G** hides all interface text.
Camera inputs take over the cinematic view without interrupting the mission.
Press **M** for continuous mouse-look, **Escape** to release the cursor, or hold right-click to turn.
With no toy selected in Free camera mode, **W/A/S/D** moves around the room; **PgUp/PgDn** changes height.
See [the gameplay state machine and Phase 2 work](docs/16-midnight-mission.md).

The room now has solid furniture and toys, a camera constrained to the room, a textured cratered moon,
more detailed characters, lamp shadows and an on-screen guide. Press **H** for the guide or **G** to hide
the interface. Select Buzz with **4**, toggle his laser with **L**, aim using **Z/X** or **Alt+click**,
and knock over the wooden blocks. **B** rebuilds the tower. You can also click a block and push it with
**W/A/S/D**. See [physics and interface details](docs/15-physics-and-interface.md).

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
| **N / Shift+N / P / Enter** | manual/progression, replay, pause, interact |
| **[ ] / , .** | clock speed / scrub in manual mode |
| **H / G** | expand corner guide / hide all text |
| **Y** | skip Penny's arrival (Shift+N replays it with the story) |

Full key list: [docs/13-controls.md](docs/13-controls.md).

## Documentation

[docs/00-index.md](docs/00-index.md) lists every chapter: pipeline basics, the vertex and index tables of
each primitive, transformation matrices, the camera, the scene graph and mounting, the characters,
illumination, shading, textures, animation, ray tracing, the code architecture, performance, the house and
Penny, and a formula-by-formula reference of one rendered frame
([docs/19-rendering-mathematics.md](docs/19-rendering-mathematics.md)).

## Project layout

```
src/        C++ sources (core, gl, math, geometry, scene, characters, world, render, app)
shaders/    GLSL shaders (lighting.glsl is shared by all of them)
assets/     BMP textures
tools/      asset generator scripts
docs/       documentation
Libraries/  GLFW, GLAD, GLM
```
