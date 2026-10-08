# Demonstration and Viva Guide

Haunted Toy Room: The Midnight Mission

Adiba Tahsin | Roll 2107031

## Start and rehearse

Run `bin/Release/HauntedToyRoom.exe` from the project folder. The arrival starts automatically. Use Y to reach the hallway puzzle quickly, Shift+N for a full replay, and G to hide the interface. Press H for help. Keep the report PDF, slide deck and Project-Demo.mp4 together for the showcase.

## Two-minute video narration

The video has visual captions and no recorded voice. Explain the following points live. The deck embeds the video on slide 2; the standalone MP4 works independently of slide playback support.

| Time | Explain |
| --- | --- |
| 0:00–0:10 | The haunted house: Penny enters at night through hinged doors and the connected staircase. |
| 0:10–0:20 | A furnished world: Five indexed primitive families form furniture and articulated toys. |
| 0:20–0:30 | Inspect the three clues: Train 2, clock 5, blocks 7. The code opens the real toy-room doors. |
| 0:30–0:42 | Rescue through hierarchy: Penny activates two switches. Jessie rides Bullseye and dismounts onto the high platform. |
| 0:42–0:52 | Buzz and the pursuit: The release removes a translucent barrier. The ghost follows Penny to the stairs. |
| 0:52–1:02 | Live character takeover: Woody follows your input while the others continue. Zero releases ownership; N enters full manual mode. |
| 1:02–1:10 | Directional, point and spot: Moonlight is directional; the lamp adds a point source and soft spotlight. |
| 1:10–1:16 | Gouraud shading: Lighting is evaluated at vertices and interpolated across triangles. |
| 1:16–1:22 | Phong shading: Interpolated normals are normalised before per-fragment illumination. |
| 1:22–1:30 | Texture coordinates: UVs map wood, wallpaper, fabric and lunar craters onto shared geometry. |
| 1:30–1:40 | Analytic ray tracing: A BVH accelerates exact primitive hits, shadows, mirror paths and straight transparency. |
| 1:40–1:52 | The entrance breaks: Buzz flies and aims. Only a nearest-hit laser impact releases six physical wood fragments. |
| 1:52–2:00 | The toys are safe: Every character is physically outside. The cast idles in the garden. Adiba Tahsin / 2107031 / CSE-4102. |

## Live demonstration sequence

1. Y skips arrival. Inspect all three hallway clues with Enter, then approach the keypad. Enter 257 and confirm with Enter. A wrong code clears the digits and keeps the door locked.
2. Penny activates the two red low switches with Enter. Select Jessie with 2, approach Bullseye and press R. Ride beneath the high platform, R dismounts onto it, and Enter activates the third switch.
3. Ctrl+0 selects Penny. Activate the red release near the rear translucent barrier. Return through the hallway, descend the stairs and press Enter at the sealed front door. Explain Buzz flight, actual nearest-hit laser impact and the six physical fragments.
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
.\bin\Release\HauntedToyRoom.exe --no-intro --manual --no-raytrace --shading 1 --select 3 --cam 0,1.5,-4.5,0,1.15,-6.6
```

## Live control design

Selection during playback owns only one actor. StoryDirector skips writes to that actor and advances a separate virtual route cursor for scene gates. Other actors continue normal waypoint motion and animation. They ignore the owned actor as an obstacle, so parking across a path cannot stall their choreography; wall and furniture contacts remain active for your actor. The door always requires a real laser hit, including when Buzz is manually controlled. The ending requires every actual character outside. Releasing control rejoins a waypoint on the current floor without teleporting. A mounted pair is detached when selected for independent control; remounting intentionally drives the connected pair. 0 releases ownership; N retains the separate full manual mode. Penny is controllable after arrival by clicking her or using Ctrl+0. The arrival camera remains cinematic until Y or arrival completion.

## Questions you should answer

## Escape implementation and objects

The keypad requires all three inspected clues and the code 257. A wrong code shows Incorrect Code and clears only the digits. Penny must approach both low switches. Jessie must have ridden Bullseye beneath the high switch, dismounted onto its platform and completed her transition before Enter activates it. The rear release belongs to Penny. At the ground-floor entrance, Enter starts Buzz's flight. The door breaks only after his nearest laser hit is the actual door for more than 0.65 seconds. Six preallocated boards receive impulses; the ghost pursues at 1.6 units per second. The ending requires the broken entrance and Penny plus all four rescued toys physically outside.

The wooden train uses box chassis and cab pieces, a horizontal cylindrical boiler, chimney and cylinder wheels. Two separate cars and a raised seven-segment 2 identify its clue. Seven coloured cubes form the block clue's 7. The hallway clock has a circular cylinder case and a thin cylinder face. The provided BMP uses planar cap UVs; a negative v repeat corrects printed orientation after rotation. A raised 5 below the face keeps the clue readable. The combination keypad has a solid wood housing, steel plate, ten raised buttons, box-strip digit glyphs and a cylinder confirmation button. Its editable three-digit display is also exposed in the HUD.

The rescue gate combines nine thin cylinders and a box rail. Its parent joint raises after two low switches and becomes hidden after the high switch. Each switch has a box backplate and a cylinder lever rotating from 20 to -45 degrees. Jessie's support platform is a solid 2.0 by 1.3 by 1.6 box; dismounting uses the existing eased rig transition. Buzz's holding area reuses the rear room floor, two box partitions and a cyan translucent barrier. Its red box release switch removes the barrier from drawing and contact tests. The entrance note is a thin paper box, accompanied by the arrival objective explaining that the toys need help.

The front-door padlock belongs to the existing hinge: one metal body box, two upright cylinders and a rotated cylinder crown move with the door. Laser impact hides this assembly and activates six wood-textured box fragments. Gravity, contact separation and angular impulses use the existing fixed-step solver. The eighteen rendered stair treads each rise 0.25 units; the support function matches each tread rather than letting a character sink through a ramp. Grounded actors remain on the correct floor, while Buzz retains vertical flight. Final rest-pose restoration preserves each root transform so the ending cannot move the cast back indoors.

Rehearse the entire escape without skipping its physical interactions:

```powershell
.\bin\Release\HauntedToyRoom.exe --no-intro --story --gameplay-demo --no-raytrace
```

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