# Toy Story: The Midnight Mission

The default automatic mode performs one complete mission, then holds the ending. It replaces independent patrol loops. Scene transitions depend on arrival and interaction state, rather than teleporting toys after arbitrary delays.

**Prologue.** Before Scene 1, Penny the cat walks home from the street, through the front door, up the
stairs and into the toy room, and jumps onto the bed to watch; the sun sets on the way
([18](18-house-and-penny.md)). **Y** skips it.

| Scene | Action and transition |
|---|---|
| 1. Discovery | Midnight. Woody looks toward the lost car beyond the obstructed doorway. |
| 2. Moving Outside | Woody walks to the exit. Jessie approaches Bullseye, mounts through the existing local/world reparenting transition, and becomes a child of his saddle. Buzz climbs and flies to the obstacle. |
| 3. Clearing the Path | Buzz aims at the crate. A real laser impulse must move it before it disappears and clears the path. |
| 4. Reaching the Car | Woody walks through the doorway; Bullseye carries Jessie; Buzz flies beside them. All three must arrive before activation. |
| 5. Activating the Car | Enter activates it, or unattended playback activates after three seconds. Its headlights illuminate and its wheels rotate as it follows a predefined route home. The friends wait clear of the route. |
| 6. Returning Home | The car parks. Woody walks home, Bullseye carries Jessie home and she dismounts, and Buzz returns and lands. |
| 7. Morning | Dawn brightens over twelve seconds, the lamp switches off, and every toy returns to its saved original pose. Penny curls up asleep on the bed. The camera pulls back, then holds the ending. |

## Taking control

- Select any toy with its number, the compact buttons or a scene click to pause the coordinated story and enter manual mode.
- **N** switches story/manual mode. Resuming continues the saved stage and waypoints. **Shift+N** restores the mission setup and replays from Discovery.
- **P** pauses the mission clock. **Enter** activates the car in Scene 5. In manual mode, select the car or bring the driven toy within three units to interact; its route then runs without steering input.
- Existing walking, riding, flying, laser, editing and camera controls remain available. Camera input overrides cinematic shots without stopping the story; N resumes cinematic camera control.
- **H** expands the guide within the same corner panel. **G** hides all interface text, including the guide. Neither key affects the scene.

The room camera remains constrained to its original boundaries. Toys can cross the right-side doorway into the hallway, whose walls, ceiling and far end remain solid. Story paths use the same character animation and collision system as manual movement.

## Implementation

`StoryDirector` owns the scene state, routes and interaction state. Mount/dismount callbacks use `ToyRoomApp`'s existing hierarchy operations. `Character::FollowWaypoint` supplies distance-based motion and animation speed; `RCCar::Animate` rotates wheels from speed and wheel radius. Saved rest transforms stop idle head, tail and limb animation at the ending. The environment's clock is controlled by the film during playback, with manual scrubbing available outside it.

Buzz uses a separate flight pose driven by actual horizontal speed, vertical movement and turning. Airborne motion suppresses the walking gait and footstep bob: cruising leans the body forward with both legs trailing, hovering stays upright, and turns bank the body. Takeoff and landing blend the poses and wing deployment smoothly. The laser wrist compensates for body lean and banking to preserve its aim.

For a repeatable accelerated scene capture:

```powershell
bin\Release\HauntedToyRoom.exe --story --story-step 0.05 --story-steps 10 --size 1000,700 --frames 240 --capture screenshots\mission-end.bmp
```

`--story-step` supplies a fixed simulation delta for captures. `--story-steps` advances multiple simulation steps per rendered frame only for story captures. Normal interactive playback uses real elapsed time. The capture log reports reached scenes and final world positions.
