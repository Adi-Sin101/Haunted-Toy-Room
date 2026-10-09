# Haunted Toy Room

A computer graphics lab project in C++ and OpenGL 3.3: **Toy Story: The Midnight Mission**.
At night Penny the cat explores the garden of an abandoned house and walks in; the main door locks
behind her. Upstairs, the train, clock and blocks give the code **257** for the Toy Room. Its old
painted toy chest has a glowing red wind-up button: Woody, Jessie and Bullseye climb out alive. Everyone
goes downstairs to an ordinary bedroom, where the wardrobe knob is too high, so Jessie rides Bullseye,
he jumps, she opens it and Buzz flies out. At the locked main door Buzz flies into position and the
player fires his laser (**L**): the door breaks into physical wood fragments. Everyone escapes, the night
turns into morning, and the world stays open for free exploration.

Two layers: you control one character (Penny, or any freed toy: 1-5 or a click) and the story layer
drives the others. **9**, or the **PENNY** button at the bottom of the corner panel, takes Penny over or
hands her back to the simulation at any time, wherever she is. Press **0** to release, **N** for full manual mode.
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

Use **Enter** near the clues, the keypad, the chest button, Buzz's door, the wardrobe and the main
door; **L** fires Buzz's laser once he is in position. The keypad requires all three clues and code
**257**. A wrong answer clears the digits. **N** switches manual/progression; **Shift+N** restarts;
**P** pauses. The optional `--gameplay-demo` rehearsal uses the same movement, interaction and collision
paths to demonstrate the whole story.
The interface is one compact corner panel: **H** expands its guide; **G** hides all interface text.
Press **0** to release the selected actor back to the story layer. Camera inputs take over the cinematic view without interrupting the mission. Mounted Jessie and Bullseye detach when selected for independent control; remounting intentionally drives the connected pair.
Press **M** for continuous mouse-look, **Escape** to release the cursor, or hold right-click to turn.
With no toy selected in Free camera mode, **W/A/S/D** moves around the room; **PgUp/PgDn** changes height.
See [the story state machine](docs/16-midnight-mission.md).

The room now has solid furniture and toys, a camera constrained to the room, a textured cratered moon,
more detailed characters, lamp shadows and an on-screen guide. Press **H** for the guide or **G** to hide
the interface. Select Buzz with **4**, toggle his laser with **L**, aim using **Z/X** or **Alt+click**,
and knock over the wooden blocks. **B** rebuilds the tower. You can also click a block and push it with
**W/A/S/D**. See [physics and interface details](docs/15-physics-and-interface.md).

Outdoors there is a real **sky dome**: a day/night gradient, a sun with its glow, a moon with maria,
twinkling stars and drifting clouds, drawn by one shared GLSL function in both the rasteriser and the
ray tracer (a ray that leaves the scene returns the sky, so reflections and glass show it too). Outside
the house the sun or moon light comes from the visible sun or moon, casts raster shadows through a
sun shadow map and ray-traced shadows through shadow rays, and the fog fades into the horizon colour.
The follow camera eases over the stair treads instead of jolting, characters are solid to each other
(long bodies use a chain of collision boxes, crowds resolve by priority and walk around each other), and
the wardrobe in Buzz's room only answers when Penny stands right in front of its doors. See
[the outdoor environment](docs/21-outdoor-sky-and-solid-characters.md).

| Keys | What happens |
|---|---|
| **1–5** | select Woody, Jessie, Bullseye, Buzz, RC car (6 ball, 7 lamp, 8 ghost, or left-click any object) |
| **9** / PENNY button | take Penny over (follow camera behind her) or hand her back to the simulation |
| **W/S/A/D**, Shift, SPACE | drive the selected toy, run, stop |
| **R** | Jessie mounts or dismounts Bullseye (walk her close to him first) |
| **Q/E**, **L** | Buzz flies and fires his laser; L also toggles the car's headlights |
| **F**, scroll, right-drag | orbit and zoom in on the selected object |
| **C** | camera mode: Free, Orbit, Follow |
| **Tab**, **T**, J/L U/O I/K | edit mode: translate, rotate, scale or shear any object |
| **F1 / F2 / F3** | wireframe, shading model, textures |
| **F4** | real-time ray tracing (shadows, reflections, transparency); **Ctrl+9** bounce count |
| **N / Shift+N / P / Enter** | manual/progression, replay, pause, interact |
| **[ ] / , .** | clock speed / scrub in manual mode |
| **H / G** | expand corner guide / hide all text |
| **Y** | skip the opening shot (Shift+N replays the story) |

Full key list: [docs/13-controls.md](docs/13-controls.md).

## Documentation

[docs/00-index.md](docs/00-index.md) lists every chapter: pipeline basics, the vertex and index tables of
each primitive, transformation matrices, the camera, the scene graph and mounting, the characters,
illumination, shading, textures, animation, ray tracing, the code architecture, performance, the house and
Penny, the [story objects and logic](docs/20-escape-gameplay.md), and a formula-by-formula reference of one rendered frame
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


## Finished showcase materials

Open [START-HERE](docs/showcase/START-HERE.md) for the report, editable 10-slide presentation, two-minute demo and learning guide. Run `Run-Showcase.ps1` to launch the Release application.
