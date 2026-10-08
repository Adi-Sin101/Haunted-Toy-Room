# Demonstration and Viva Guide

Haunted Toy Room: The Midnight Mission

Adiba Tahsin | Roll 2107031

## Start and rehearse

Run `bin/Release/HauntedToyRoom.exe` from the project folder. The story opens with a crane shot over the street; Y skips it. You then control Penny in the garden. Shift+N replays the story and G hides the interface. Press H for help. Keep the report PDF, slide deck and Project-Demo.mp4 together for the showcase.

## Two-minute video narration

The video has visual captions and no recorded voice. Explain the following points live. The deck embeds the video on slide 2; the standalone MP4 works independently of slide playback support.

| Time | Explain |
| --- | --- |
| 0:00–0:08 | Night outside the house: Penny arrives at the abandoned house. The camera cranes down from the street. |
| 0:08–0:18 | In through the open door: Penny walks in; the main door closes and locks. She climbs the eighteen-step stair. |
| 0:18–0:26 | Three clues: 257: Train 2, clock 5, blocks 7. The keypad opens the real Toy Room doors. |
| 0:26–0:36 | The toy chest: The red wind-up button opens the lid. Woody, Jessie and Bullseye climb out alive. |
| 0:36–0:46 | Downstairs together: The toys follow Penny through a graph of doorway nodes and down the stairs. |
| 0:46–0:58 | Cupboard rescue: Jessie mounts Bullseye, he jumps, she opens the wardrobe and Buzz flies out. |
| 0:58–1:10 | The locked main door: Penny tries the door. Buzz flies into position; L fires a nearest-hit laser and the door breaks. |
| 1:10–1:22 | Morning escape: Everyone leaves the house. The fog thins, the sun rises and the camera pulls back. |
| 1:22–1:30 | Free exploration: The story stays open in daylight. Adiba Tahsin / 2107031 / CSE-4102. |
| 1:30–1:38 | Live character takeover: Woody follows your input while the others keep accompanying Penny. Zero releases ownership. |
| 1:38–1:44 | Gouraud shading: Lighting is evaluated at vertices and interpolated across triangles. |
| 1:44–1:50 | Phong shading: Interpolated normals are normalised before per-fragment illumination. |
| 1:50–2:00 | Analytic ray tracing: A BVH accelerates exact primitive hits, shadows, mirror paths and straight transparency. |

## Live demonstration sequence

1. Y skips the opening shot. Walk Penny (W/S/A/D) through the gate, up the porch and in at the open front door: it closes and locks behind her. Climb the stairs.
2. Inspect the train, clock and blocks with Enter, then use the keypad: 257 and Enter. A wrong code clears the digits and keeps the door locked.
3. Walk to the toy chest and press Enter at its glowing red button. The lid opens; Woody, Jessie and Bullseye climb out. They follow Penny from now on.
4. Go downstairs. Press Enter at the door on the left of the corridor. In Buzz's room press Enter at the wardrobe: the knob is too high, so Jessie rides Bullseye, he jumps and she opens it. Buzz flies out. (Or select Jessie with 2, R to mount, ride beside the wardrobe and L to jump.)
5. Lead everyone to the main entrance and press Enter: it is locked. Buzz flies into position; press L to fire. Walk outside: the night turns into morning and the camera pulls back. Free exploration follows.
6. 5 selects the car. W/S and A/D drive, L toggles moving spotlights. F focuses it; hallway camera access is supported.
7. 7 selects the lamp. R toggles power, W/S tilts, A/D swivels, comma/period alters intensity. Both light position and direction follow the rig.
8. F2 cycles Flat/Gouraud/Phong/Blinn and selects the raster path. F5/F6/F7 isolate terms. F3 toggles colour textures. F4 selects analytic ray tracing; 9 changes bounces and minus/equal changes resolution.
9. Click a toy or furniture. Tab enters edit mode; T chooses Translate/Rotate/Scale/Shear. J/L, U/O and I/K change axes. M mirrors, Backspace restores. V lists actual parts; Shift+V prints vertices/indices.
10. O enables haunted ambience. Show the ghost's opacity, ball rolling and lamp motion. N toggles full manual mode; Shift+N restarts the story.

Reproducible rider setup:

```powershell
.\bin\Release\HauntedToyRoom.exe --no-intro --manual --mount --select 1 --no-raytrace
```

Reproducible matched shading view:

```powershell
.\bin\Release\HauntedToyRoom.exe --no-intro --manual --no-raytrace --shading 1 --select 3 --cam -2.0,1.5,2.4,-3.4,1.15,0.4
```

## Live control design

The player layer owns one character; the story layer drives the rest. Freed toys accompany Penny in fixed formation slots and route between floors through thirteen doorway nodes (Floyd-Warshall next steps). Taking one toy over removes it from the group without moving the others. Jessie and Bullseye perform the wardrobe rescue on their own unless you control one of them; then you do it with R, riding and L. Buzz flies into position at the locked door and waits for your L. The door always requires a real laser hit, including when Buzz is manually controlled. Toys still inside the chest or wardrobe cannot be selected. A mounted pair is detached when selected for independent control; remounting drives the connected pair. 0 releases ownership; N switches to full manual mode; Shift+N replays the story.

## Questions you should answer

## Story implementation and objects

Every stage advances only on its real condition. Penny must actually walk through the open front door before it locks. The keypad requires all three inspected clues and 257; a wrong code shows Incorrect Code and clears only the digits. The chest opens when Enter is pressed within 3.2 units of it. Buzz's door opens with Enter beside it on the ground floor. The wardrobe opens only when a mounted Bullseye is within 1.4 units of the spot beside it and his jump lift exceeds 0.45: Penny's Enter there asks Jessie and Bullseye for help, or the player can mount (R), ride and jump (L) themselves. The main door must be tried (Enter, or standing at it for one second). It breaks only after Buzz's nearest laser hit has been the real door leaf for more than 0.65 seconds. Morning starts when Penny steps outside; free exploration follows once the sixteen-second sunrise has finished and the camera has pulled back over the house.

The wooden train uses box chassis and cab pieces, a horizontal cylindrical boiler, chimney and cylinder wheels. Two separate cars and a raised seven-segment 2 identify its clue. Seven coloured cubes form the block clue's 7. The hallway clock is wall-sized: a 0.8-unit circular cylinder case hung at eye height and a thin cylinder face. The provided BMP uses planar cap UVs; a negative v repeat corrects printed orientation after rotation. A small brass plate below the face carries a raised 5. The combination keypad has a solid wood housing, steel plate, ten raised buttons, box-strip digit glyphs and a cylinder confirmation button. Its editable three-digit display is also exposed in the HUD.

The toy chest is hollow (floor board and four solid walls) with iron brackets and bands, brass rivets, a keyhole plate and a front panel carrying a procedural painted texture: worn red planks, stars, clouds and a rocket. Its lid is a hinge joint holding a deep frame box and a cylinder dome whose lower half lies inside the frame, so the lid stays curved when open. The red wind-up button pulses and a red story light follows it. Pressing it swings the lid to -105 degrees; Woody, Jessie and Bullseye climb to the rim and hop out with the eased transition (a 0.4 sin(pi t) arc), then cheer. Buzz's bedroom under the Toy Room is ordinary: star wallpaper, a patchwork star quilt, a bookcase with a globe, rocket and teddy bear, a nightstand lamp (the room's light), a star rug and a football. The wardrobe has a hollow carcass, an arched crown with finials and two hinged doors with arched panels, cut-out gold stars and knobs 2.65 units up, too high for Penny; a green glow leaks from the gap while Buzz is inside.

Bullseye's jump lifts his body joint, and so the saddle and a mounted Jessie, by 0.75 sin(pi t) over 0.9 s while his root stays on the floor; Jessie's reach pose raises her right arm. Buzz then flies out on four waypoints. At the entrance the padlock rides on the door hinge; the laser hides the door and releases six wood boards that the fixed-step solver drops onto the porch deck.

Rehearse the entire story without skipping its physical interactions:

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

### How does Bullseye's jump lift Jessie to the wardrobe knob?

The jump offsets his body joint by 0.75 sin(pi t) over 0.9 s. The saddle and Jessie are descendants of that joint, so they rise with it; his root stays on the floor for physics.

### How do the toys follow Penny between floors?

Each part of the house is a zone joined by doorway nodes. In Penny's zone a toy walks to its formation slot; elsewhere it walks to the next node on the Floyd-Warshall shortest route toward her.

### How does the chest lid stay curved when open?

The lid is a hinge joint holding a deep frame box and a half-visible cylinder dome. The dome's lower half lies inside the frame, so only the curved top is ever visible.

### What makes the morning?

The story drives the clock from 23:36 to 07:00 in sixteen seconds. The same hour moves the sun, blends sky and ambient colours and scales the fog density by the night factor.

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