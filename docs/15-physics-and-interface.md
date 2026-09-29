# Physics, scene detail and on-screen controls

The room is now 20 by 18 units, with a 7.5-unit ceiling. A doorway in the right wall connects to a bounded hallway for the Midnight Mission. Character collisions permit crossing through this opening; the camera remains inside the room. The initial camera starts inside it. The bed, bookcase, window frame, desk, lamp and wooden blocks have collision bounds. Character proxies cover moving limbs, hats, the horse's head and tail, and Buzz's open wings. Jessie shares Bullseye's larger proxy while mounted.

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

The lunar texture is generated on spherical coordinates at 1024 by 512 pixels, with 110 crater bowls and rims, dark maria, fine grain and baked sunlight. Surface noise is continuous at the sphere's texture seam. Cotton, denim, leather, plaid and cow-print textures are available in both renderers. The asset library's spheres now use 32 stacks and 48 sectors. The room adds curtains, bedding, books, desk items and trim. Toy details include fingers, faces, garment buttons, stitching, tack, tyre treads and space-suit joints.

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
