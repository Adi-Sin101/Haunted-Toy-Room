# Physics, scene detail and on-screen controls

The room is now 20 by 18 units, with a 7.5-unit ceiling, and it is the upper floor of a house
([18](18-house-and-penny.md)). A doorway in the right wall (with a double door) connects to a bounded
hallway for the Midnight Mission; the hallway's side wall has a door at the top of the stairs, and the
collision bounds keep the toys in the hallway. Character collisions permit crossing through this opening; the camera remains inside the room. The initial camera starts inside it. The bed, bookcase, window frame, desk, lamp and wooden blocks have collision bounds. Character proxies cover moving limbs, hats, the horse's head and tail, and Buzz's open wings. Jessie shares Bullseye's larger proxy while mounted.

`PhysicsWorld` sweeps movement against expanded bounds instead of checking only the final position. This stops fast movement through thin obstacles and allows sliding along furniture. The same solver keeps Free, Orbit and Follow cameras inside the room and clear of solid objects. Obstructed orbit/follow views shorten toward the target. If an edit puts the camera inside an object, it moves to a clear face without crossing a room boundary. Very large edited toys are scaled to fit the room.

Blocks use gravity at a fixed 120 Hz, projected rotated bounds, collision impulses, restitution, floor friction and angular damping. A block can fall off a stack, slide and tumble after being struck. This is a small conservative simulation: contacts use bounding boxes rather than a complete oriented rigid-body contact solver. Clothing decorations are covered by each character's physical proxy rather than individual finger colliders.

Buzz's beam follows his wrist and ends at its first contact. An exact primitive ray finds wooden blocks; furniture and the room boundary stop the beam. Repeated hits apply linear and angular impulses.

## Controls added or improved

| Control | Action |
|---|---|
| H | Open or close the on-screen guide |
| G | Show or hide the interface |
| Click a toy name | Select it from any viewpoint |
| Click a block | Select it; W/A/S/D push relative to the camera and Space brakes |
| Ctrl+B or the Blocks toolbar button | Cycle block selection from any viewpoint |
| B | Rebuild the tower and clear block velocities |
| 4, then L | Select Buzz and switch his laser on/off |
| Z / X with Buzz selected | Lower / raise the wrist's aim |
| Alt + left-click/drag with Buzz selected | Aim at a visible surface or block |
| Ctrl + scroll | Adjust lens zoom in any camera mode |
| Right-drag / arrows in Follow | Change the following view's angle |
| Middle-drag in Orbit / Follow | Pan; the offset persists until selection, F or Home resets it |

The interface is rendered after the 3D scene, including ray tracing, so text and controls stay the same size when zooming. It is a single compact corner panel, showing the clock, current story scene or selected toy, relevant controls and camera mode. H expands the guide in that same corner; G hides the entire interface. Fonts are rasterized once with Windows GDI and drawn using OpenGL; `gdi32.lib` is the only additional system dependency.

The lunar texture is generated on spherical coordinates at 1024 by 512 pixels, with 110 crater bowls and rims, dark maria, fine grain and baked sunlight. Surface noise is continuous at the sphere's texture seam. Cotton, denim, leather, plaid and cow-print textures are available in both renderers. Spheres, cylinders and cones come in three levels of detail that are chosen by on-screen size (see [17 - Performance](17-performance.md)). The room adds curtains, bedding, books, desk items and trim. Toy details include fingers, faces, garment buttons, tack and space-suit joints. Fine surface detail such as stitching, braiding, tyre grain and quilting comes from textures instead of extra shapes.

Normal rendering includes a 2048-pixel lamp depth map with nine shadow samples per shaded point. Flat, Phong and Blinn use fragment visibility; Gouraud samples visibility per vertex to preserve that shading model. Ray tracing continues to use analytic shadow rays, reflections and transparency.

## Verification

Run focused physics checks from the VS Code terminal:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-physics.ps1
```

The checks cover tunnelling, sliding, overlap recovery, room boundaries, character separation, edited scale, stable stacking, laser impulses, tumbling, floor clearance, rebuilding, laser occlusion and camera sightlines.

Scripted captures exercise the actual scene:

```powershell
bin\Release\HauntedToyRoom.exe --laser-demo --frames 180 --capture screenshots\laser.bmp
bin\Release\HauntedToyRoom.exe --guide --size 1000,700 --frames 5 --capture screenshots\guide.bmp
bin\Release\HauntedToyRoom.exe --raytrace --frames 3 --capture screenshots\raytraced.bmp
```

`--no-hud` creates an unobstructed scene capture. Capture directories are created automatically.

## House, garden and Buzz's bedroom bounds

With the house enabled, the ground floor is a union of four boxes, each shrunk by the mover's rotated
half-size: the corridor, the stair landing, Buzz's bedroom and the bedroom doorway. A point outside all
of them moves to the nearest box. A solid divider wall (with the doorway in it) and the bedroom's door
leaf decide where characters actually pass. Outdoors (story prologue, and after the main door breaks)
the bounds cover the garden, pavement and street; the front walls, fence, gate posts, porch columns,
railings, shrubs and tree trunks are solid. `FloorHeight` adds the porch deck and its three steps.

The corridor and the stair flight are both places to stand and used to be separated only by two
zero-thickness planes; a thin solid `StairDivider` between them makes the wall physical.

**Characters are solid.** While the story's followers walk, hard actor contacts are off so the group
cannot jam in a doorway; walls, doors, furniture and blocks still stop everyone. After all moves,
`PhysicsWorld::SeparateActors` resolves every overlap between characters by **priority** (the driven
character, then story tasks, then Penny, then the followers): the lower body yields the whole push,
sliding against the house; pinned beside a doorway it steps aside along the other axis; whatever it still
cannot give, the higher body gives back. The driven character never moves. Story walkers also steer
around bodies ahead of them.

**Spines.** Penny (tail -0.8 to nose +1.0) and Bullseye (rump -0.9 to muzzle +1.7) use chains of small
square boxes along the body instead of one box. A single long box would swell by up to 41 % at 45° and
could not turn in the 3-unit corridor; square segments stay compact at any heading. `SlideActor` sweeps
all segments with the same rotation and keeps the most restrictive result, so the body stops when its nose
touches a wall. Before, both animals' heads passed through walls.

The camera never collides with characters (they cannot hide the view), only with the house. Broken door
boards come to rest on the porch and are pushed aside by characters rather than blocking them. Details,
figures and the new physics checks: [21](21-outdoor-sky-and-solid-characters.md).

## Penny control in the corner panel

Under the selection buttons, the **PENNY** row shows whether you drive her (*YOU CONTROL HER*) or the
simulation does (*SIMULATION*); a click or **9** toggles it. Taking her over puts the follow camera behind
her, wherever she is. (Ray-tracing bounces moved from 9 to **Ctrl+9**.)

