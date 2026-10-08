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
| 0 | clear selection (N resumes the paused mission) |
| Left click | the object under the mouse (ray picking) |

The selected object pulses with a golden rim light. F11 toggles its local axes. The on-screen toolbar also selects toys; Ctrl+B or the Blocks button cycles block selection even when the blocks are outside the current view. H opens the guide and G hides the interface.

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

Buzz's laser applies impulses to wooden blocks. Use Z/X to lower/raise his aim or Alt+left-click/drag to aim at a visible surface. B rebuilds the block tower. With a block selected, W/A/S/D push relative to the camera and Space brakes it.

## Props

| Object | Keys |
|---|---|
| Desk lamp (7) | A/D swivel, W/S tilt, R power on/off, `,` / `.` brightness |
| Ball (6) | W/S/A/D push relative to the camera, SPACE stop |

## Camera

Press **M** outside edit mode for GTA-style mouse-look: moving the mouse turns the view continuously
with no button held and no screen-edge limit. **Escape** releases the cursor without changing the view;
pressing Escape again with the cursor released quits. Holding right-click also captures the mouse until
released. Opening render settings with the backtick key releases the cursor for clicking. Losing window
focus releases capture and clears held movement keys.

With **no toy selected** (press 0) and **Free** camera mode, **W/A/S/D** walks the camera relative to its
view; **PgUp/PgDn** changes height and **Shift** moves faster. Selected toys retain their movement keys.
Use **C** to cycle camera modes: mouse-look also orbits the target in Orbit and Follow modes. The camera
remains constrained to the room and solid objects. Mouse input takes over the cinematic view without
pausing the mission. **G** hides the interface and center aiming marker.

| Key / mouse | Free mode | Orbit mode | Follow mode |
|---|---|---|---|
| C | cycle Free → Orbit → Follow | | |
| F | focus: orbit around the selected object | | |
| Home | reset camera | | |
| Arrow keys | move forward/back/strafe | rotate around target | change viewing angle |
| PgUp / PgDn | move up / down | closer / further | closer / further |
| Right-drag | look around | rotate around target | change viewing angle |
| Middle-drag | pan | pan the target | pan the target |
| Scroll | zoom (field of view) | closer / further (dolly) | closer / further |
| Ctrl + Scroll | lens zoom | lens zoom | lens zoom |
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

## Rendering settings

The small Settings button at the top right opens four independent switches: Lighting, Shading,
Ray tracing and Textures. All four start on. The backtick key opens or closes the menu; G hides or restores
the entire interface, including its settings button. Toggling rendering does not pause the mission.
Shift+F2 toggles surface shading; F2 enables it or cycles the retained shading model. F3 and F4 still
toggle textures and ray tracing. Lighting off shows plain material colours. Shading off skips face
lighting, specular highlights and shadows, while enabled lighting can still supply light colour and
distance attenuation. Textures off uses flat preview colours so room surfaces remain distinguishable.

## Prologue (Penny's arrival)

| Key | Action |
|---|---|
| Y | skip the arrival and begin the hallway puzzle with the entrance closed |
| Shift+N | replay the arrival and the story from the start |
| G / H | hide the interface / expand the guide (as always) |

During the arrival the camera, selection and object controls are disabled; the clock and the camera are
driven by the arrival.

## Penny's hallway puzzle

After the arrival, Penny is automatically selected as the player character and starts in the upper-floor
hallway. Her movement uses the existing character controller and hallway collision bounds; character
selection is held on Penny until the puzzle is solved.

| Key | Action |
|---|---|
| W / S | move Penny forward / backward |
| A / D | turn Penny left / right |
| SPACE | stop Penny |
| ENTER near a clue | inspect the toy train, old clock or colored blocks |
| ENTER near the keypad | start keypad input |
| 0-9 | enter keypad digits |
| BACKSPACE | delete the last digit |
| ENTER while entering | submit the three-digit code |
| ESC while entering | cancel keypad mode; Penny movement resumes |

The HUD shows the interaction prompt when Penny is close enough, clue progress, and the code. All three
clues must be inspected before `257` unlocks the Toy Room door. A wrong answer clears the digits and allows
another attempt. Character selection is available again after entering the Toy Rescue stage.

## World

| Key | Action |
|---|---|
| P | pause / resume the mission and clock |
| [ / ] | time speed ×½ / ×2 |
| , / . | scrub time backward / forward (brightness instead when the lamp is selected) |
| N | switch manual control / gameplay progression |
| Shift+N | replay Penny's arrival and reset the gameplay state machine |
| Enter | interact with a story object when an interaction is implemented |
| H | expand / close the compact corner guide |
| G | hide / show all interface text |
| Esc | quit |

## Command-line options (scripted runs / screenshots for the report)

```
HauntedToyRoom.exe [--hour 21.5] [--select N] [--focus] [--orbit yaw,pitch,dist] [--cam x,y,z,tx,ty,tz]
                   [--mount] [--raytrace] [--shading 0..3] [--wireframe] [--normals] [--pause] [--story]
                   [--capture file.bmp] [--frames 90] [--benchmark 400] [--intro] [--no-intro] [--no-raytrace]
```

`--select` takes the 0-based object index (0 Woody … 7 Ghost). With `--capture`, the clock is paused, the
story is disabled for deterministic poses (unless `--story` is given), a screenshot is written after `--frames` frames and the program
exits. Example:

```
HauntedToyRoom.exe --hour 12 --select 2 --mount --orbit 60,15,5 --capture mounted.bmp
```

`--no-raytrace` starts in raster mode (ray tracing is on by default, like lighting, shading and
textures). `--no-intro` starts directly with the story (Penny is already on the bed); `--intro` keeps Penny's arrival
in a scripted capture (captures skip it otherwise). Example: `--intro --no-hud --story-step 0.05 --frames 160
--capture garden.bmp` shows Penny on the garden path 8 s into the arrival.

`--benchmark N` turns v-sync off, skips 60 warm-up frames, times the next N frames and prints the
average frame time, draw calls and triangles, then exits (see [17 - Performance](17-performance.md)).
