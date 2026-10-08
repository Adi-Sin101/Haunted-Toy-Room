# Haunted Toy Room gameplay progression

`src/world/StoryDirector.*` owns the single gameplay state machine. `PennyArrival` remains responsible for
the original arrival route, environment transition, and arrival camera. When that prologue completes (or is
skipped with **Y**), the director enters Stage 1 and the entrance closes behind Penny.

## States and transitions

| State | Objective | Transition condition |
|---|---|---|
| `PROLOGUE` | Penny's arrival | Arrival completes or is skipped |
| `PUZZLE` | Find the code: 3 clues | Puzzle is solved |
| `TOY_RESCUE` | Free the Toys | Rescue switches are activated |
| `BUZZ_RESCUE` | Free Buzz | Buzz's release mechanism is activated |
| `FINAL_ESCAPE` | GET EVERYONE OUT | The escape completes |
| `WIN` | THE TOYS ARE SAFE / YOU ESCAPED | Terminal ending state |

`StoryDirector::Advance` accepts guarded transition events. An event advances the director only from its
matching state; out-of-order events are ignored. State queries expose puzzle/rescue availability, chase
activity, Toy Room lock status, final-door seal status, and ending status so gameplay systems can bind to
the progression without creating a second story manager.

## Implemented Stage 1 puzzle

`HallwayPuzzle` builds a two-car toy train, a wall clock displaying five, seven colored blocks arranged
as a seven, and a physical keypad from the project's shared primitive meshes and materials. Their solid
shapes join the existing physics scenery and render through both raster and ray-tracing traversal.
Approach a clue and press **Enter** to inspect it. Approach the keypad, press **Enter**, enter three digits,
then press **Enter** to submit; **Backspace** removes the last digit and **Esc** closes the keypad. The
player must inspect all three clues before a correct submission unlocks the door and advances the state.
- Select a toy with its number, the compact buttons or a scene click for live control while other actors continue. Press **0** to release it. **N** selects full manual mode.
- **N** switches story/manual mode. Resuming continues the saved stage and waypoints. **Shift+N** restores the mission setup and replays from Discovery.
- **P** pauses the mission clock. **Enter** activates the car in Scene 5. In manual mode, select the car or bring the driven toy within three units to interact; its route then runs without steering input.
- Existing walking, riding, flying, laser, editing and camera controls remain available. Camera input overrides cinematic shots without stopping the story; N resumes cinematic camera control.
- **H** expands the guide within the same corner panel. **G** hides all interface text, including the guide. Neither key affects the scene.

The camera can inspect the connected hallway while preserving closed wall boundaries. Toys can cross the right-side doorway into the hallway, whose walls, ceiling and far end remain solid. Story paths use the same character animation and collision system as manual movement.

Remaining work includes Toy Room rescue switches, Jessie and Bullseye's high-switch sequence, Buzz's room
and release interaction, hostile chase behavior, escape routes, final-door collision/destruction, Buzz's
laser hit on the door, and the exterior ending camera and house-darkening sequence. Those systems should
trigger the existing guarded state transitions while reusing the scene hierarchy, character movement,
Buzz flight and laser, physics, environment, and camera systems already in the project.

Manual character selection/control, object selection, and camera modes remain available through the
existing controls. **N** switches manual/progression control; **Shift+N** restarts the arrival and state
machine; **Y** skips the arrival.
