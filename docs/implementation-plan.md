# Haunted Toy Room — Implementation Plan

## Context
Graphics-lab OpenGL project (C++ / GLFW / GLAD GL 3.3 core / GLM, Visual Studio 2026 v145, x64).
`docs/project-context.md` defines the concept: a child's toy room comes alive at night; Toy Story–style
characters (Woody, Jessie, Bullseye, Buzz, RC Car) built from primitives, individually selectable and
controllable (1–5, W/S/A/D, SPACE stop, R mount/unmount), Jessie mounts Bullseye via parent–child
hierarchy with no teleporting; must demonstrate L3 (3D transformations), L8 (illumination), L9 (shading).
User adds: hand-authored vertices/indices/triangles for every shape, own lighting/shading, **GPU ray-traced
mode**, **procedural textures + own BMP loader** (no image library), full camera + per-object inspection
control (teacher may ask to zoom/move anything), self-moving environment, and complete step-by-step docs.
Cast kept small; everything built from a few reusable shared meshes and reusable builders.

Current state (verified):
- `Project1.slnx/.vcxproj/.filters/.user`, sources flat in root: `Main.cpp` (one quad), `shaderClass.*`,
  `VAO.*`, `VBO.*`, `EBO.*`, `glad.c`, `default.vert/.frag`, `ChatGPT.cpp` (cube/pyramid/sphere sample, not in project).
- **Broken**: IncludePath/LibraryPath hard-coded to `C:\Users\Nihal\source\repos\Project1\Libraries\...`
  (Debug only; Release has none and no glfw link). `Libraries/lib/glfw3.lib` = x64, `/MDd` (MSVCRTD).
- `Shader::compileErrors` exists but never called; compares `type != "PROGRAM"` by pointer (bug).
- `VBO` takes raw `GLfloat*` only; vertex = pos+color (no normals/UVs).
- Stale artifacts: `x64/`, `Project1/x64/`, `.vs/`. No git repo. Bullseye code referenced in doc is not in repo → built fresh.

## Phase 0 — Fix environment, rename, reorganize
1. Rename solution/project: `Project1.*` → `HauntedToyRoom.slnx/.vcxproj/.vcxproj.filters/.vcxproj.user`;
   update slnx Path, `RootNamespace`, keep ProjectGuid.
2. vcxproj (all configs, x64 primary; drop Win32 — lib is x64 only):
   - `IncludePath = $(ProjectDir)Libraries\include;$(ProjectDir)src`, `LibraryPath = $(ProjectDir)Libraries\lib`.
   - Link `glfw3.lib;opengl32.lib` in Debug and Release; Release: `RuntimeLibrary=MultiThreadedDLL` + ignore
     `MSVCRTD` (`IgnoreSpecificDefaultLibraries`) so the /MDd lib links cleanly; W4, C++20.
   - `OutDir = $(ProjectDir)bin\$(Configuration)\`, `IntDir = $(ProjectDir)build\$(Configuration)\`.
   - Post-build copy of `shaders/` and `assets/` next to exe; `.user` sets `LocalDebuggerWorkingDirectory=$(ProjectDir)`.
3. New layout (filters mirror folders):
   ```
   src/main.cpp
   src/core/     Application (window, loop, delta time, resize), Input (key edge detection, mouse, scroll)
   src/gl/       Shader, VAO, VBO, EBO (existing classes kept + upgraded), Texture, Framebuffer
   src/math/     Transform3D (hand-written translate/scale/rotX/Y/Z/axis-angle/shear/reflect/lookAt/perspective)
   src/geometry/ Vertex{pos,normal,uv}, Mesh, Primitives (plane, cube, sphere, cylinder, cone)
   src/scene/    Transform(TRS), SceneNode (tree), MatrixStack, Material, Light, Camera, Scene
   src/characters/ Character base (controls, walk cycle), HumanoidBuilder (Woody/Jessie/Buzz), Bullseye, RCCar
   src/world/    Room (floor, walls, window, desk, lamp, ball, ceiling fan), DayNightCycle, StoryDirector
   src/render/   Renderer (raster), RayTracer (GPU), ProceduralTextures, BmpLoader, Picking
   shaders/      lit.vert/.frag (Flat/Phong/Blinn), gouraud.vert/.frag, raytrace.vert/.frag, debug(lines/points)
   assets/textures/*.bmp   (few 24-bit BMPs, e.g. poster, window sky)
   docs/         numbered guides (below)
   ```
4. Move `glad.c` → `src/gl/glad.c`; port `generateSphere` idea from `ChatGPT.cpp` into Primitives, then delete
   `ChatGPT.cpp`, `Main.cpp`, `default.*`. Delete stale `x64/`, `Project1/`, `.vs/`.
5. `git init` + `.gitignore` (`.vs/ bin/ build/ *.user`). No commit unless asked.
6. Build with MSBuild (`C:\Program Files\Microsoft Visual Studio\18\Community`) Debug+Release x64, run exe → window opens.

## Engine design (reuse + efficiency rules)
- **5 shared meshes only** (unit-sized, centered: cube [-0.5,0.5]³, sphere r=0.5, cylinder r=0.5 h=1, cone r=0.5 h=1,
  plane 1×1 XZ). Every object = SceneNode tree referencing those meshes + Material. One VAO per primitive, uploaded once.
- Vertex = pos(3) + normal(3) + uv(2), interleaved stride 32 B; `VBO` gains `std::span<const Vertex>` ctor; EBO `GLuint`.
- Cube uses 24 vertices / 36 indices (4 per face) — documented why 8 shared vertices can’t hold per-face normals/UVs.
- Shader: compile/link errors checked and printed; uniform-location cache (`unordered_map`); `setMat4/setVec3/...`.
- Classes own GL handles (move-only, destroy in destructor) — no manual Delete leaks.
- SceneNode: `Transform local`, `Mesh*`, `Material*`, `children`, `worldMatrix` computed in one traversal with
  MatrixStack (push/pop), matching lecture style. Same traversal feeds raster draw list and ray-tracer instance list.
- No per-frame heap allocation (reuse vectors), delta-time everything, uniforms per-frame vs per-draw separated.

## Phase 1 — Primitives from manual vertices/indices (docs 02, 03)
- Plane, cube hand-typed tables; sphere (stacks/sectors lat-long), cylinder (side ring + caps with center vertex),
  cone (side with slant normals + base cap) generated by explicit loops with formulas for pos/normal/uv and
  CCW index winding. `glEnable(GL_CULL_FACE)` validates winding.
- Debug views: F1 wireframe (`glPolygonMode`), F9 vertex points + normal lines, `V` dumps selected mesh’s
  vertex/index table to console (show triangles live to the teacher).

## Phase 2 — Transformations, hierarchy, camera (L3; docs 04, 05)
- `math/Transform3D`: matrices written element-by-element from lecture formulas (translation, scaling about pivot,
  rotation X/Y/Z, rotation about arbitrary axis (Rodrigues), shear, reflection, composite order), glm only as
  storage/multiply; also own `lookAt`, `perspective`.
- MatrixStack + SceneNode hierarchy; local vs world coordinates.
- Camera modes (C cycles): **Free-fly** (arrow keys move, PgUp/PgDn up/down, right-mouse drag look),
  **Orbit** around selected object (mouse drag rotate, scroll zoom distance — close inspection),
  **Follow** (third-person behind selected). Scroll = zoom (FOV in free mode), F = focus selected,
  Numpad 1/3/7 front/side/top presets, Home = reset.
- Object inspector (Tab toggles EDIT mode on selected object): X/Y/Z-cycle operation Translate/Rotate/Scale
  with `I/K` (±X), `J/L` (±Z), `U/O` (±Y); Backspace resets. Works on any node (characters, lamp, ball, furniture).

## Phase 3 — Room + characters + control (docs 06, 07)
- Room: floor, 3 walls, window (sky quad showing moon/sun), desk, desk lamp (light source), ball, ceiling fan,
  bed optional only if cheap. All from shared primitives.
- `HumanoidBuilder(params)` builds Woody, Jessie, Buzz from one parametric tree (head, eyes, nose, hair, hat,
  torso, arms→hands, legs→boots; params = colors, proportions, hair style, extras). Buzz extra: helmet sphere,
  wings (cubes), laser (thin emissive cylinder). Bullseye: quadruped builder (body, neck, head, ears, 4 legs, tail,
  saddle node). RCCar: body, cabin, 4 wheels (cylinders rotating by distance/radius), headlights (spotlights).
- Selection: 1 Woody, 2 Jessie, 3 Bullseye, 4 Buzz, 5 Car, 6 Ball, 7 Lamp, 0 camera-only; mouse left-click picks
  via ray–primitive intersection (shared with ray tracer). Selected object highlighted (outline/tint).
- Controls (selected): W/S move along heading, A/D turn, SPACE stop, delta-time; walk cycle via
  `sin(t·speed)·amp` on legs/arms, returns to neutral on stop. Buzz Q/E fly up/down, L laser. Car H headlights.
- Jessie R mount/unmount: distance check; mount = reparent to Bullseye saddle node with short lerp to seat;
  unmount = world = parent·local, placed beside Bullseye, heading kept. Mounted: 2 controls Bullseye unit.
  Edge-detected keys (1–7, R, SPACE, Tab, toggles). Console prints only on state change.
- HUD: window title shows `SELECTED | mode | shading | time of day | FPS`; H prints full help to console.

## Phase 4 — Illumination (L8; doc 08)
- Material {ambient, diffuse, specular, shininess, emissive, texture, reflectivity}.
- Lights (uniform arrays): directional moon/sun, point lamp bulb (constant/linear/quadratic attenuation),
  spotlight lamp cone (inner/outer cutoff), car headlights spot. Toggles: F5 ambient, F6 diffuse, F7 specular,
  F8 cycle light enable, `,`/`.` lamp intensity.

## Phase 5 — Shading models (L9; doc 09)
- F2 cycles Flat (face normal from `dFdx/dFdy` in frag) / Gouraud (lighting per vertex, separate shader) /
  Phong (per-fragment, reflect) / Blinn-Phong (half-vector). Same light/material uniforms across shaders.
- Normal matrix = transpose(inverse(model)) computed CPU per draw (explained for non-uniform scale).

## Phase 6 — Textures (doc 10)
- `ProceduralTextures`: wood-plank floor, striped wallpaper, checkerboard toy blocks, star night-sky, fabric —
  texels computed on CPU (formulas documented), uploaded with mipmaps.
- `BmpLoader`: hand-written 24/32-bit BMP parser (header fields, row padding, bottom-up, BGR→RGB).
- Texture class: wrap/filter params, mipmaps; UV mapping per primitive documented; F3 texture on/off; texture ×
  lighting modulation.

## Phase 7 — Environment motion & story (doc 11)
- DayNightCycle: sun/moon direction orbit, sky/ambient color blend, window sky texture swap/blend.
- Lamp flicker (layered sine noise) at night, ceiling fan rotation, ball rolling (rotation axis = up × velocity,
  angle = distance / radius), curtains sway optional.
- StoryDirector (N toggles): night → unselected toys autopilot on simple waypoint loops; morning → return to
  home positions and freeze. Manual control always overrides. P pause world, `[` `]` time speed.

## Phase 8 — GPU ray tracing mode (doc 12)
- F4 toggles. Each frame the scene traversal emits instance list: primitive type, inverse world matrix, material
  (color, texture id, specular, reflectivity) → packed into RGBA32F **texture buffer** (GL 3.3 core OK).
- Full-screen quad frag shader: camera ray per pixel → transform ray to each instance’s object space → analytic
  unit cube (slab), sphere, cylinder (with caps), cone, plane intersection → world normal via
  `transpose(mat3(invModel))` → Phong with same lights, **shadow rays**, **reflection bounces** (floor/ball/helmet,
  iterative depth 2–3), procedural UV for floor/walls. Rendered to half-res FBO, upscaled; `-`/`=` resolution scale.
- CPU copy of intersection routines used for mouse picking.

## Documentation (written alongside each phase, `docs/`)
`00-index.md`, `01-environment-setup.md` (libs, linking, paths, build/run), `02-opengl-pipeline-fundamentals.md`
(VBO/VAO/EBO, attributes, stride/offset, shaders, NDC, depth test, culling, winding),
`03-primitives.md` (full vertex+index tables per shape, triangle diagrams, formulas, worked sphere example with
small stacks/sectors), `04-transformations.md` (every matrix with lecture formula + code), `05-camera.md`
(view/projection derivation, modes, controls), `06-scene-graph-hierarchy.md` (MatrixStack, mount/unmount math),
`07-characters.md` (per character: hierarchy tree, each part’s primitive/local TRS/color/material table),
`08-illumination.md`, `09-shading.md`, `10-textures.md`, `11-environment-animation.md`, `12-ray-tracing.md`,
`13-controls.md` (complete key map), `14-code-architecture.md`.

## Verification (each phase)
1. MSBuild `HauntedToyRoom.slnx` Debug|x64 and Release|x64 — zero errors, warnings reviewed.
2. Launch `bin/Debug/HauntedToyRoom.exe` from project dir; check console for shader compile/link errors; confirm
   window renders (kill after smoke test).
3. Manual checklist from project-context §20: selection 1–5, W/S/A/D, SPACE stop, walk anims, R mount when close /
   "too far" when far, Jessie follows Bullseye move+rotate, dismount without teleport, Bullseye intact; plus
   camera modes/zoom, edit mode on any object, F1–F9 toggles, day/night cycle, ray-trace mode shadows/reflections,
   mouse pick.
