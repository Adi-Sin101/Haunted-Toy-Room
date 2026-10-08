# Demonstration and Viva Guide

Haunted Toy Room: The Midnight Mission

Adiba Tahsin | Roll 2107031

## Start and rehearse

Run `bin/Release/HauntedToyRoom.exe` from the project folder. The arrival starts automatically. Use Y to reach the toy room quickly, Shift+N for a full replay, and G to hide the interface. Press H for help. Keep the report PDF, slide deck and Project-Demo.mp4 together for the showcase.

## Two-minute video narration

The video has visual captions and no recorded voice. Explain the following points live. The deck embeds the video on slide 2; the standalone MP4 works independently of slide playback support.

| Time | Explain |
| --- | --- |
| 0:00–0:10 | Welcome home: Penny's route links the garden, doorway, stairs and toy room. |
| 0:10–0:20 | A furnished world: Five reusable primitives build the house, furniture and articulated toys. |
| 0:20–0:32 | Hierarchy in motion: Jessie attaches to Bullseye's saddle; one parent transform moves both. |
| 0:32–0:44 | Flight, laser and collision: Buzz aims at the obstacle. The nearest ray hit receives an impulse. |
| 0:44–0:56 | Rescue and return: The car follows waypoints. Wheel rotation uses distance divided by radius. |
| 0:56–1:06 | Live character takeover: Woody follows your controls while Jessie and Buzz continue the mission; 0 releases him, N enters full manual. |
| 1:06–1:14 | Directional, point and spot: Moonlight is directional. The lamp adds a point source and a soft cone. |
| 1:14–1:20 | Gouraud shading: Light is evaluated at vertices; the rasterizer interpolates the result. |
| 1:20–1:26 | Phong shading: Interpolated normals are normalized; the reflection-vector highlight is per pixel. |
| 1:26–1:34 | Procedural textures: UVs map grain, wallpaper, fabric and lunar craters onto shared geometry. |
| 1:34–1:46 | Ray tracing: A BVH finds analytic hits; shadow rays and mirror bounces reveal the polished floor. |
| 1:46–1:52 | A quiet morning: The lamp turns off, the toys return home and Penny falls asleep. |
| 1:52–2:00 | Thank you: Adiba Tahsin | Roll 2107031 | CSE-4102 |

## Live demonstration sequence

1. Y skips arrival; 1 selects Woody for live takeover. Hold W, turn with A/D, stop with Space. Jessie and Buzz continue their routes. Press 0 to release Woody; press N for full manual mode. Explain separate input ownership and scene-clock progression.
2. 2 selects Jessie. R is distance-gated at 2.2 units. Move her near Bullseye or use the reproducible mounted launch below, then R. Select either rider or horse and drive; R dismounts.
3. 4 selects Buzz. Q/E moves vertically, L toggles the laser, Z/X aims, B rebuilds blocks. Explain the emitter hierarchy, nearest-hit test and impulse.
4. 5 selects the car. W/S and A/D drive, L toggles moving spotlights. F focuses it; hallway camera access is supported.
5. 7 selects the lamp. R toggles power, W/S tilts, A/D swivels, comma/period alters intensity. Both light position and direction follow the rig.
6. F2 cycles Flat/Gouraud/Phong/Blinn and selects the raster path. F5/F6/F7 isolate terms. F3 toggles colour textures. F4 selects analytic ray tracing; 9 changes bounces and minus/equal changes resolution.
7. Click a toy or furniture. Tab enters edit mode; T chooses Translate/Rotate/Scale/Shear. J/L, U/O and I/K change axes. M mirrors, Backspace restores. V lists actual parts; Shift+V prints vertices/indices.
8. O enables haunted ambience. Show the ghost's opacity, ball rolling and lamp motion. N resumes the coordinated story; Shift+N restarts it.

Reproducible rider setup:

```powershell
.\bin\Release\HauntedToyRoom.exe --no-intro --manual --mount --select 1 --no-raytrace
```

Reproducible matched shading view:

```powershell
.\bin\Release\HauntedToyRoom.exe --no-intro --no-raytrace --shading 1 --select 3 --cam 0.4,1.5,0.5,0.4,1.15,-1.8
```

## Live control design

Selection during playback owns only one actor. StoryDirector skips writes to that actor and advances a separate virtual route cursor for scene gates. Other actors continue normal waypoint motion and animation. They ignore the owned actor as an obstacle, so parking across a path cannot stall their choreography; wall and furniture contacts remain active for your actor. Buzz takeover uses a three-second doorway gate without changing your laser or aim. A mounted pair is detached when selected for independent control; remounting intentionally drives the connected pair. 0 releases ownership; N retains the separate full manual mode. Penny is controllable after arrival by clicking her. The arrival camera remains cinematic until Y or arrival completion.

## Questions you should answer

### Why 24 cube vertices instead of eight?

A geometric corner belongs to three faces. Each face needs a distinct normal and UV chart, so the indexed mesh keeps four vertices per face.

### What is the difference between illumination and shading?

Illumination computes light at one point. Shading decides where that computation is evaluated and which quantities are interpolated.

### Why can Gouraud miss a highlight?

It evaluates specular response only at vertices. A peak between vertices is absent from the interpolated values.

### Why normalise a Phong normal after interpolation?

Interpolation does not preserve unit length. The cosine dot product requires a unit vector.

### Why inverse transpose for normals?

It preserves perpendicularity to transformed tangents under non-uniform scale and shear: (A^-T n)·(A t)=n·t.

### What happens when shininess increases?

The exponent suppresses values below one more strongly, producing a narrower specular highlight. ks controls its strength.

### Why are the spotlight cutoffs cosine values?

The cone test uses a dot product of unit vectors. The outer angle is larger, so its cosine is smaller; smoothstep fades between these bounds.

### How does Jessie follow Bullseye?

Her root is attached under the saddle. The horse world matrix multiplies the saddle and rider local matrices, so translation, rotation and gait propagate naturally.

### Why do you not normalise the transformed ray direction?

Keeping its length preserves the same t in object/world space. Hits from different scaled instances can then be compared directly.

### Is the tracer testing every triangle?

No. The BVH bounds scene instances, then tests exact plane, cube, sphere, cylinder and cone equations in object space.

### Is transparency the same as refraction?

No. This implementation continues straight through with reduced throughput. It does not apply Snell's law.

### How are raster shadows different from ray shadows?

Raster lamp shadows compare a receiver with the lamp depth map. Ray shadows query whether an opaque object lies before the selected light.

### Why retain off-screen geometry in the ray scene?

It may still appear in a reflection or occlude a light. Camera frustum culling is applied only to the raster submission.

### Does a colour texture change geometry?

No. It modulates albedo. The fence alpha is coverage, while the moon craters and cloth weave remain colour detail rather than displacement or normal mapping.

### How are wheel and ball angles calculated?

Angle equals distance/radius. A ball uses an axis perpendicular to up and displacement; wheels rotate around their fixed axle.

### How do optimisations preserve the appearance?

Shared buffers preserve identical geometry; LOD retains more triangles when large on screen; conservative bounds prevent false culling; texture detail replaces subpixel objects; bounded simulation prevents unbounded catch-up work.

### What are the deliberate limitations?

Lamp-only general raster shadow map; selected/thresholded ray shadows; finite continuations; straight transparency; box-approximate contacts. No global diffuse transport or physical refraction.

## Parameter-change practice

| Parameter | Location | Expected effect |
| --- | --- | --- |
| `shininess` / `ks` | `Room.cpp` material creation or `Material.h` default | Narrower highlight / brighter highlight |
| `linear`, `quadratic` | `ToyRoomApp::BuildScene` light setup | Faster dimming with distance |
| 22° / 34° lamp cone | `ToyRoomApp::BuildScene` cutoff values | Inner lit region / larger smooth cone |
| `reflectivity` | `Room.cpp` floor/ball materials | Stronger mirror contribution in tracer |
| `rayBounces`, `rayScale` | `RenderSettings.h`; keys 9 and minus/equal | Deeper continuation / more pixels and cost |
| `maxSpeed`, `turnRate` | `Character.h` | Faster motion / faster turning |
| Sphere stacks/sectors | `Assets::Load` | Smoother silhouette but more triangles |
| `uvScale` | Material creation in room/house builders | More/fewer pattern repeats |
| Fan 110 degrees/s | `ToyRoomApp::StepScene` | Faster/slower rotor animation |

## One frame: code reading route

Read `Application::Run` → `ToyRoomApp::OnUpdate` / `StepScene` → input handlers → `StoryDirector::Update` and `Character::Animate` → `PhysicsWorld` → `SceneNode::UpdateWorld` → `UpdateLights` → `OnRender` → `Renderer::Render` or `RayTracer::Render`. Read `lighting.glsl` once, then compare `gouraud.vert`, `lit.frag` and `raytrace.frag`. Look at the named object's builder and inventory row when explaining a part. You do not need to memorise every line.

## Checks and reproduction

```powershell
.\tools\build.ps1 -Configuration Release
.\tools\build.ps1 -Configuration Debug
.\tools\check-physics.ps1
python tools/check_showcase_interaction.py
python tools/check_showcase_live.py
python tools/make_showcase.py --capture --video
python tools/build_showcase_docs.py
```

Document generation requires pdfLaTeX (MiKTeX or TeX Live) as well as Python packages listed in `tools/showcase-requirements.txt`. The final report source is `Project-Report.tex`; `python tools/build_latex_report.py` rebuilds the PDF alone. Existing artifacts are ready to use; regeneration is optional. `validation/` contains build/check, interaction, story, capture and media evidence. `inventory/` is the complete node/material/surface-map data used for the report.