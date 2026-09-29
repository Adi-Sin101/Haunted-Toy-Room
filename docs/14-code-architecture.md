# 14 — Code Architecture

## 1. Layers

```
               main.cpp
                  │
            app/ToyRoomApp ─────────────── input handling, selection, camera control, inspector,
                  │                        mount/dismount, lights ← scene nodes, HUD, capture
   ┌──────────────┼──────────────────┬──────────────────────┐
 world/         characters/        render/                 core/
 Room            Character          Renderer (raster)        Application (window + loop)
 Environment     Humanoid, Buzz     RayTracer (GPU)          Input, Paths
 StoryDirector   Bullseye, RCCar    Assets, DebugLines
                                    ProceduralTextures, BmpLoader, Image
   └──────────────┴────────┬─────────┴──────────────────────┘
                        scene/                 geometry/              math/          gl/
                        SceneNode, Transform   Vertex, Mesh,          Transform3D,   Shader, VAO, VBO,
                        MatrixStack, Camera,   Primitives             Ray            EBO, Texture, glad
                        Material, Light
```

Lower layers never include higher ones: `gl/` knows nothing about the scene, `scene/` nothing about
characters, characters nothing about the application.

## 2. Responsibilities

| Module | Owns | Key functions |
|---|---|---|
| `core/Application` | GLFW window, GL context, main loop, FPS | `Run`, virtual `OnUpdate(dt)`, `OnRender` |
| `core/Input` | key / mouse state from callbacks | `Down`, `Pressed` (edge), `MouseDelta`, `Scroll` |
| `core/Paths` | locating `shaders/` and `assets/` | `resolve` |
| `gl/*` | one OpenGL object each (RAII, move-only) | `Bind`, `Upload`, `Set*` uniforms |
| `geometry/Primitives` | vertex + index generation | `Plane`, `Cube`, `Sphere`, `Cylinder`, `Cone` |
| `geometry/Mesh` | GPU copy + CPU copy of a primitive | `Draw`, `DrawPoints`, `Data` |
| `math/Transform3D` | hand-written matrices | `translate`, `rotateX/Y/Z`, `rotateAxis`, `shear`, `reflect`, `lookAt`, `perspective`, `normalMatrix` |
| `math/Ray` | ray–primitive intersection (CPU) | `RayIntersect::Object` |
| `scene/SceneNode` | the hierarchy, world matrices | `AddChild`, `AddShape`, `AttachChild`, `DetachChild`, `UpdateWorld` |
| `render/Assets` | 5 meshes, all textures, all materials (by name) | `Load`, `Mat`, `SlotTexture` |
| `render/Renderer` | draw list, raster passes, debug overlay | `Collect`, `Render`, `RenderDebug`, `UploadLights` |
| `render/RayTracer` | instance buffer, FBO, trace + present | `Render` |
| `characters/Character` | movement, walk phase, autopilot, transitions | `Drive`, `Stop`, `SteerTowards`, `StartTransition` |
| `world/Room` | building the room and props | `BuildRoom` → `RoomRig` handles |
| `world/Environment` | clock, sky, lamp, ball, ghost | `Update`, `Scrub`, `PushBall`, `DriveLamp` |
| `world/StoryDirector` | coordinated seven-scene Midnight Mission and car autopilot | `Init`, `Restart`, `Update`, `Pause` |

## 3. Ownership and lifetime

* `ToyRoomApp` owns `Assets`, `Renderer`, `RayTracer`, the scene root (`unique_ptr<SceneNode>`) and the
  characters (`vector<unique_ptr<Character>>`).
* Scene nodes own their children (`unique_ptr`); re-parenting moves the `unique_ptr` (mount/dismount), so
  there is never a leak or a double owner.
* Characters and selectables hold **non-owning** pointers into the scene graph; nodes are never deleted
  while the program runs.
* GPU objects free themselves in destructors. Destruction order follows declaration order in reverse, and
  the GLFW window/context (in the `Application` base) is destroyed **after** all members that hold GL
  objects.

## 4. Efficiency decisions

| Decision | Benefit |
|---|---|
| 5 shared meshes for the whole scene | tiny GPU memory, no per-object buffers |
| Static VBO/EBO uploaded once | no per-frame buffer traffic (only the debug lines and ray-tracer instance buffer are dynamic) |
| Cached uniform locations | no string lookups in the driver per draw |
| Material upload skipped when consecutive items share a material | fewer uniform calls |
| Reused `std::vector`s (draw list, instance buffer, debug lines) | no per-frame heap allocation |
| One world-matrix pass per frame | each matrix computed once, shared by raster, ray tracer, picking and lights |
| Back-face culling | ~half the fragments of closed objects skipped |
| Ray tracer: analytic primitives + per-object bounding spheres + reduced resolution | real-time ray tracing on a GL 3.3 GPU |
| Frame-rate independent motion (`dt`, clamped to 0.1 s) | same speed on every machine, no jump after a stall |

## 5. Adding a new object (the pattern)

1. Write a builder that creates nodes with `AddChild` (joints) and `AddShape` (parts) using the shared
   meshes from `Assets` and materials from `assets.Mat(name, colour, ks, shininess)`.
2. If it moves under control, derive from `Character`, set `maxSpeed / turnRate`, and override `Animate`
   (and optionally `Special`, `TurnFactor`, `OnDrive`).
3. Register it in `ToyRoomApp::BuildScene` with `AddSelectable(name, root, character, key, focusHeight,
   focusDistance)` and, if it should live in the story, `story.Add(character, waypoints)`.

Everything else — highlighting, picking, edit mode, camera focus, ray tracing, geometry dumps — works
automatically for the new object.

## 6. Coding conventions

* C++20, warning level 4, zero warnings in Debug and Release.
* One class per header/source pair; small headers, implementation in `.cpp`.
* Comments explain the **graphics concept** being implemented (formulas, matrix layouts, why an order
  matters), not what a line of C++ does.
* Shaders share code through `#include "lighting.glsl"` so the three render paths cannot drift apart.
