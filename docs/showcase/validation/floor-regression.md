# Hallway and stair floor correction — 11 October 2026

## Reproduced faults

1. The actual hallway's one-sided plane did not obstruct the follow-camera sightline. A target at `(15,0.5,2)` and desired eye at `(15,-1,0.5)` crossed underneath it without shortening. The actual tread cubes also did not obstruct a vertical camera sightline. A check building the existing `BuildRoom`/`BuildHouse` scene failed before the correction.
2. Penny's segment-centre support did not cover a swinging paw's reach. Sweeping the configured spine over the 18 treads, with front/rear reach ±0.80, reproduced 0.238 world units of penetration into a higher tread.
3. After full-footprint support was added, Bullseye's rear segment still did not cover a backward hoof swing. The humanoid checks passed; Bullseye's reversed-heading check failed. Extending his rear segment from -0.35 to -0.60 made both pass.

## Changed source files and functions

| File | Change |
| --- | --- |
| `src/scene/SceneNode.h` | Add `cameraSolid` metadata for rendered support surfaces |
| `src/world/Room.h` | Define shared `RoomSize::StairSteps = 18` |
| `src/world/Room.cpp` — `BuildRoom` | Use 0.30-thick cubes with top Y=0 for main/upper hallway floors; preserve materials and top-face UV orientation |
| `src/world/House.cpp` — `BuildHouse` | Use 0.20-thick corridor/landing slabs with unchanged top Y; extend corridor to `HouseFront`; register all treads as camera obstacles; use shared stair count |
| `src/world/PhysicsWorld.h` | Store camera-only scenery/bounds and add an internal camera-contact option to `Move` |
| `src/world/PhysicsWorld.cpp` — `Init`, `RefreshScenery` | Collect/refresh visible camera-only bounds separately from actor/block contacts |
| `src/world/PhysicsWorld.cpp` — `FloorHeight` | Use the same stair count as geometry construction |
| `src/world/PhysicsWorld.cpp` — `SlideActor` | Support grounded actors over segment footprint centres/edges, including the upper threshold; do not apply this lift to ungrounded flight |
| `src/world/PhysicsWorld.cpp` — `Move`, `MoveCamera`, `CameraSightline` | Include floor/tread camera bounds in the existing sweep/sightline solver |
| `src/app/ToyRoomApp.cpp` — `BuildScene` | Extend Bullseye's rear spine segment to cover hoof swing |
| `tests/PhysicsChecks.cpp` | Add `CheckStairFloorContact`, `CheckToyStairFeet`, `CheckRenderedHouseFloors`; invoke them from `main` |
| `tests/PhysicsChecks.vcxproj` | Link real house/room/assets and their existing texture dependencies into the regression executable |

Documentation additions: `Teacher-Evaluation-QA.md` with 130 numbered questions and 348 checked source references, plus printable PDF and editable DOCX versions; links in `START-HERE.md`; this validation note. The Q&A describes the current eight-state chest/wardrobe story and distinguishes earlier plans from implemented behaviour.

No renderer, shader, ray-tracer, texture-loader, character model or story-progression implementation was replaced. Floor cubes enter the same scene traversal as every other primitive. Camera-only flags use `PhysicsWorld`'s existing bounds/sweep implementation; they do not add a second collision engine or make stair risers block character ascent.

## Validation results

| Check | Result |
| --- | --- |
| x64 Debug build | Passed |
| x64 Release build | Passed |
| Physics/puzzle/math/follower regression suite | 117 checks passed |
| Penny proxy ascending/descending at 0°, 45°, 90°, 180° | Passed |
| Humanoid boot and Bullseye hoof reach at both tested headings | Passed |
| Actual floor/tread camera sightline and sweep | Passed |
| All 18 tread camera bounds | Passed |
| Actual floor cube top and underside analytic hits | Passed |
| Corridor-floor continuity to entrance | Passed |
| Full story replay: Flat/Gouraud/Phong/Blinn | Passed |
| Full story replay: ray tracing and Debug | Passed |
| Real laser door impact and five actual characters outside in FREE_EXPLORE | Passed in all six replay configurations |

Commands: `tools/build.ps1 -Configuration Debug`, `tools/build.ps1 -Configuration Release`, `tools/check-physics.ps1`, `python tools/check_escape_rendering.py`.

Running Debug screenshots were inspected in the normal follow camera at stair ascent/upper threshold and descent. Examples are in the ignored runtime `screenshots/` directory:

- `stairs-fixed-raster.bmp` and `stairs-fixed-ray.bmp` — ascent/top threshold.
- `stairs-toys-fixed-raster.bmp` and `stairs-toys-fixed-ray.bmp` — descent toward the lower landing.

These are captures from executing the game and its deterministic rehearsal. The rehearsal uses controller waypoints, the existing interactions and actual physics; it does not teleport characters or bypass story conditions. This is **automated gameplay plus visual screenshot inspection**, not a claim that a human manually played every possible route/camera angle. Existing `escape-*.log` files hold the latest complete-replay evidence.

## Limits and environment

Support remains conservative and moves the whole grounded root. It prevents foot/riser penetration but is not independent per-foot IK; feet over a lower tread can have some clearance. Edited poses/scales and every possible camera angle have not been exhaustively tested.

The system C: drive ran out of space during a later test build. Compiler temporary files were redirected, for those commands only, to `build/floor-check-temp` on E:. The final builds/checks passed with that setting; no unrelated files were deleted to free space.

After the user freed disk space, C: had approximately 5.6 GB available. Both builds and all 117 checks were rerun successfully with the normal temporary-file configuration. The complete six-mode escape replay passed again, and the 130 questionnaire entries, 348 exact source references, 22-page PDF and editable Word export were rechecked. The running Debug game was closed before rebuilding to release its executable lock.

Fresh Debug captures at rehearsal time 31 seconds (`screenshots/floor-final-raster.bmp` and `screenshots/floor-final-ray.bmp`) were also visually inspected. Both show the stair treads and lower landing rendered beneath Penny in the normal Follow Camera. This repeats the automated gameplay and screenshot inspection described above.
