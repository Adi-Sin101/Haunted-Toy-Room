# 13 — Complete Controls Reference

Press **H** in the program to print this list to the console. The window title always shows the clock,
the selected object, edit mode, camera mode, render mode and FPS. The console reports state changes
(selection, moving/stopped, mount/dismount, toggles) — once per change, never every frame.

## Selecting

| Key | Selects |
|---|---|
| 1 | Woody |
| 2 | Jessie (Jessie + Bullseye while mounted) |
| 3 | Bullseye |
| 4 | Buzz |
| 5 | RC car |
| 6 | Beach ball |
| 7 | Desk lamp |
| 8 | Ghost |
| 0 | nothing (camera only; all toys follow the story) |
| Left click | the object under the mouse (ray picking) |

The selected object pulses with a golden rim light and shows its local axes (X red, Y green, Z blue).

## Driving the selected character

| Key | Action |
|---|---|
| W / S | move forward / backward along the current heading |
| A / D | turn left / right |
| Shift | run (2× speed) |
| SPACE | stop immediately (hold to stay stopped) |
| R | Jessie / Bullseye: mount / dismount (Jessie must be within 2.2 units of Bullseye) |
| Q / E | Buzz: fly up / down |
| L | Buzz: laser on/off · RC car: headlights on/off |

While Jessie is mounted, W/S/A/D/SPACE drive Bullseye and Jessie rides along.

## Props

| Object | Keys |
|---|---|
| Desk lamp (7) | A/D swivel, W/S tilt, R power on/off, `,` / `.` brightness |
| Ball (6) | W/S/A/D push relative to the camera, SPACE stop |

## Camera

| Key / mouse | Free mode | Orbit mode | Follow mode |
|---|---|---|---|
| C | cycle Free → Orbit → Follow | | |
| F | focus: orbit around the selected object | | |
| Home | reset camera | | |
| Arrow keys | move forward/back/strafe | rotate around target | — |
| PgUp / PgDn | move up / down | closer / further | — |
| Right-drag | look around | rotate around target | — |
| Middle-drag | pan | pan the target | — |
| Scroll | zoom (field of view) | closer / further (dolly) | closer / further |
| Shift | 3× faster | | |
| Numpad 1 / 3 / 7 or Ctrl+1 / 3 / 7 | front / side / top view (switches to Orbit) | | |

## Edit mode (inspector) — works on every selectable object

| Key | Action |
|---|---|
| Tab | toggle edit mode (W/S/A/D driving is paused while editing) |
| T | cycle operation: Translate → Rotate → Scale → Shear |
| J / L | −X / +X (Rotate: yaw) |
| U / O | +Y / −Y (Rotate: roll) |
| I / K | −Z / +Z (Rotate: pitch) |
| Ctrl (Scale) | uniform scale |
| M | mirror (reflection about the local YZ plane) |
| Backspace | reset the object's transform |
| Shift | 3× faster |

## Rendering and debugging

| Key | Action |
|---|---|
| F1 | wireframe (shows every triangle) |
| F2 | shading: Flat → Gouraud → Phong → Blinn-Phong |
| F3 | textures on/off |
| F4 | **ray tracing** on/off |
| - / = | ray tracing resolution down / up |
| 9 | ray tracing bounces 0–4 |
| F5 / F6 / F7 | ambient / diffuse / specular term on/off |
| F8 | sun/moon light on/off |
| F9 | normals of the selected object |
| F10 | vertices of the selected object |
| F11 | local axes gizmo on/off |
| F12 | screenshot → `screenshots/shot_NNN.bmp` |
| V | list the selected object's parts (primitive, local position/size, world position, counts) |
| Shift+V | also print every vertex and triangle of the primitives it uses |

## World

| Key | Action |
|---|---|
| P | pause / resume the clock |
| [ / ] | time speed ×½ / ×2 |
| , / . | scrub time backward / forward (brightness instead when the lamp is selected) |
| N | story + haunting on/off (toys wander at night, return home in the morning) |
| H | print help |
| Esc | quit |

## Command-line options (scripted runs / screenshots for the report)

```
HauntedToyRoom.exe [--hour 21.5] [--select N] [--focus] [--orbit yaw,pitch,dist] [--cam x,y,z,tx,ty,tz]
                   [--mount] [--raytrace] [--shading 0..3] [--wireframe] [--normals] [--pause] [--story]
                   [--capture file.bmp] [--frames 90]
```

`--select` takes the 0-based object index (0 Woody … 7 Ghost). With `--capture`, the clock is paused, the
story is disabled for deterministic poses (unless `--story` is given), a screenshot is written after `--frames` frames and the program
exits. Example:

```
HauntedToyRoom.exe --hour 12 --select 2 --mount --orbit 60,15,5 --capture mounted.bmp
```
