# 07 — Characters and Props: How Each Object Is Built

Every object is a tree of scene nodes (see [06](06-scene-graph-hierarchy.md)) whose shapes are the five
primitives of [03](03-primitives.md). Positions and sizes below are in world units: the room is
**20 wide × 18 deep × 7.5 high** (x ∈ [−10, 10], z ∈ [−9, 9]), with a doorway in the right wall leading
into a hallway that reaches x = 17. A humanoid toy is about 2.1 units tall.

In the trees below, "pos" is the shape's local position relative to its parent joint, "size" is the local
scale applied to the unit primitive, and "rot" is Euler degrees (pitch, yaw, roll).

All characters face **+Z** in their own space, stand with their feet at y = 0 and have their left side on
**+X**. The heading rotates the whole tree about Y.

To see the real numbers live: select an object and press **V** (list of parts with local and world
positions) or **Shift+V** (plus all vertex/index tables). `--benchmark` prints the measured shape and
triangle count of every object:

| Object | Shapes | Triangles at full detail |
|---|---|---|
| Woody | 62 | 50 328 |
| Jessie | 63 | 53 628 |
| Buzz | 76 | 63 828 |
| Bullseye | 44 | 28 264 |
| RC Car | 32 | 6 360 |
| Ball / Desk lamp | 1 / 5 | 1 656 / 3 632 |
| Penny the cat | 34 | 34 656 |
| Room and scenery (story / arrival with the house) | 97 / 232 | 15 558 / 57 402 |

(With level of detail the *drawn* count is a fraction of this — see [03 §8](03-primitives.md) and
[17](17-performance.md).)

## 1. Design for reuse and for performance

* **One humanoid builder** (`Humanoid`, `src/characters/Humanoid.cpp`) makes Woody, Jessie and Buzz from a
  `HumanoidStyle` (colours + options: vest, hat, long hair braid, space-ranger parts).
* Helper functions `BuildLeg` and `BuildArm` build one limb under a joint; they are called twice per
  character.
* **One `Character` base class** (`src/characters/Character.*`) provides driving, delta-time movement,
  acceleration, the walk phase, autopilot steering, home position and smooth transitions for all five
  controllable toys. Each subclass only adds its model and its own `Animate`.
* **Materials are shared by name** (`Assets::Mat`): e.g. `eye-white`, `eye-pupil`, `brass` are one material
  each, used by every character. The renderer sorts opaque draws by material, so sharing also means fewer
  state changes.
* **Detail comes from textures, not from extra shapes** (two rounds: [17](17-performance.md)). The second
  round removed the shirt buttons, pockets, eye glints, Buzz's seal rings and vent, the car's stripes and
  handles and the bridle buckles, merged the four finger cylinders of each hand into one block, turned
  Woody's 5-cube star badge into a disc and the 24 books into 4 textured boxes.
* **Earlier:** Stitching, the braid weave, tyre grain and
  saddle studs used to be hundreds of tiny spheres and cubes; they are now fabric/leather/denim textures on
  one shape (see [10](10-textures.md)). Shapes that remain are the ones that change the *silhouette*
  (ears, the hat brim, the scarf, the holster, one finger block per hand).

## 2. Humanoid (Woody, Jessie, Buzz)

```
Root (feet on floor, heading)
└─ Pelvis                  joint  pos (0, 0.85, 0)            bobs while walking
   ├─ Belt                 cube   pos (0, 0.03, 0)      size (0.40, 0.12, 0.25)
   ├─ Buckle               cube   pos (0, 0.03, 0.13)   size (0.11, 0.08, 0.03)
   ├─ Holster              cube   pos (−0.24, −0.10, 0) size (0.09, 0.23, 0.15) rot (0,0,−12)  (Woody)
   ├─ LeftHip              joint  pos ( 0.11, 0, 0)           rotX = walk swing
   │  ├─ Leg               cyl    pos (0, −0.31, 0)     size (0.15, 0.62, 0.15)     denim texture
   │  ├─ Knee              sphere pos (0, −0.34, 0.035) size (0.155, 0.16, 0.13)
   │  ├─ BootShaft         cyl    pos (0, −0.67, 0)     size (0.18, 0.22, 0.18)     leather texture
   │  ├─ Foot              cube   pos (0, −0.80, 0.05)  size (0.18, 0.10, 0.30)
   │  ├─ RoundedToe        sphere pos (0, −0.78, 0.16)  size (0.18, 0.09, 0.13)
   │  └─ Sole              cube   pos (0, −0.848, 0.055) size (0.19, 0.025, 0.32)
   ├─ RightHip             joint  pos (−0.11, 0, 0)           (same children)
   └─ Torso                joint  pos (0, 0.09, 0)
      ├─ Chest             cube   pos (0, 0.25, 0)      size (0.42, 0.50, 0.24)     plaid (Woody) / cotton
      ├─ VestLeft/Right    cube   pos (±0.13, 0.27, 0.125) size (0.15, 0.44, 0.02)   cow print (Woody)
      ├─ VestBack          cube   pos (0, 0.27, −0.125) size (0.43, 0.44, 0.02)
      ├─ Neck              cyl    pos (0, 0.54, 0)      size (0.10, 0.10, 0.10)
      ├─ Collar ×2         cube   pos (±0.075, 0.47, 0.14) size (0.13, 0.10, 0.025) rot (0,0,±24)
      ├─ Neckerchief       cyl    pos (0, 0.53, 0)      size (0.17, 0.055, 0.17)
      ├─ ScarfKnot, ScarfTail  sphere / cone at the front of the neck
      ├─ SheriffBadge      cyl    pos (0.14, 0.39, 0.145) size (0.09, 0.015, 0.09) rot (90,0,0)  (Woody: a brass disc)
      ├─ LeftShoulder      joint  pos ( 0.27, 0.46, 0)        rotX = −0.8·swing, rotZ = 6°
      │  ├─ UpperArm       cyl    pos (0, −0.14, 0)     size (0.12, 0.28, 0.12)
      │  ├─ ShoulderBall   sphere pos (0, 0, 0)         size 0.13
      │  ├─ Elbow          sphere pos (0, −0.29, 0)     size 0.12
      │  ├─ Forearm        cyl    pos (0, −0.41, 0)     size (0.105, 0.23, 0.105)
      │  ├─ Cuff           cyl    pos (0, −0.50, 0)     size (0.125, 0.06, 0.125)
      │  ├─ Hand           sphere pos (0, −0.57, 0)     size (0.13, 0.14, 0.09)
      │  ├─ Fingers        sphere pos (−0.002, −0.63, 0.022) size (0.12, 0.09, 0.06)   one rounded mitten block
      │  └─ Thumb          sphere pos (0.068, −0.565, 0.025) size (0.06, 0.065, 0.05)
      ├─ RightShoulder     joint  pos (−0.27, 0.46, 0)        (same children)
      └─ Head              joint  pos (0, 0.56, 0)            idle: rotY look-around, rotX nod
         ├─ Skull          sphere pos (0, 0.20, 0)      size (0.34, 0.38, 0.34)
         ├─ EyeLeft/Right  sphere pos (±0.07, 0.24, 0.145) size (0.08, 0.09, 0.05)   white
         ├─ Iris ×2        sphere pos (±0.07, 0.24, 0.168) size (0.046, 0.052, 0.014) brown / green
         ├─ PupilL/R       sphere pos (±0.07, 0.24, 0.165) size (0.04, 0.05, 0.03)
         ├─ Eyebrow ×2     cube   pos (±0.072, 0.303, 0.146) size (0.079, 0.016, 0.019) rot (0,0,±9)
         ├─ Ear ×2         sphere pos (±0.17, 0.20, 0)  size (0.07, 0.11, 0.055)
         ├─ Cheek ×2       sphere pos (±0.095, 0.14, 0.12) size (0.09, 0.075, 0.06)
         ├─ Nose           sphere pos (0, 0.18, 0.17)   size 0.055
         ├─ ChinDetail     sphere pos (0, 0.04, 0.11)   size (0.17, 0.075, 0.09)
         ├─ Mouth          cube   pos (0, 0.10, 0.15)   size (0.10, 0.02, 0.03)
         ├─ Hair           sphere pos (0, 0.27, −0.035) size (0.36, 0.32, 0.35)
         ├─ Braid (Jessie) joint  pos (0, 0.22, −0.17)  rot (−12, 0, 0)
         │  ├─ Plait      cyl    pos (0, −0.28, 0)     size (0.12, 0.56, 0.12)   woven texture ×(2, 6)
         │  ├─ Bow        sphere pos (0, −0.02, −0.02) size (0.16, 0.09, 0.09)
         │  └─ Tip        sphere pos (0, −0.58, 0)     size 0.12
         ├─ HatBrim        cyl    pos (0, 0.38, 0)      size (0.66, 0.03, 0.66)
         ├─ HatCrown       cyl    pos (0, 0.49, 0)      size (0.30, 0.20, 0.30)
         └─ HatBand        cyl    pos (0, 0.42, 0)      size (0.31, 0.04, 0.31)
```

*Height check:* feet 0 → hips 0.85 → chest top 0.85 + 0.09 + 0.50 = 1.44 → head joint 1.50 → hat top
1.50 + 0.49 + 0.10 = 2.09.

*Limb length check:* leg top at the hip (y = 0.85), the sole's underside at
0.85 − 0.848 − 0.0125 ≈ −0.01 ≈ the floor. The arm hangs 0.14 + … down to the finger tips at
−0.635 − 0.0325 ≈ −0.67 below the shoulder (y = 0.85 + 0.09 + 0.46 − 0.67 ≈ 0.73 above the floor).

### Styles

| | Woody | Jessie | Buzz |
|---|---|---|---|
| shirt | yellow plaid | white cotton | white |
| vest | white cow-print | red yoke (cotton) | green side panels |
| pants | blue denim | blue denim | white |
| boots | brown leather | tan leather | purple |
| hair | short brown | red + braid with yellow bow | none (purple hood) |
| hat | brown cowboy hat | red cowboy hat | none (glass helmet) |
| eyes | brown iris | green iris | brown iris |

### Buzz extras (`class Buzz : Humanoid`)

```
Head  + Hood (purple sphere behind the face), Chin (purple), Helmet (sphere 0.6, glass: opacity 0.22,
        reflectivity 0.25 → drawn in the transparent pass, reflective in ray tracing)
Torso + ChestPlate, ChestStripe, three buttons (red/green/blue), RangerBadge
Each shoulder + ShoulderArmor, WristBand                   Each hip + KneeArmor
      + Wings joint pos (0, 0.33, −0.16): Pack, 2 Thrusters with rims, WingLeft/Right (cube 0.75×0.06×0.22,
        rolled ±8°) and red wing tips
        scale.x of the joint = 0.08 (folded) … 1.0 (open) — opens when Buzz is airborne
RightShoulder + LaserEmitter (red cylinder), LaserBeam (thin emissive cylinder, up to 6 long), LaserTip anchor
```

**L** toggles the laser: the right arm blends up to −90° pitch (pointing forward), the beam appears and a
red point light follows the laser tip. **Q / E** fly up / down.

![Buzz](images/buzz.png)

### Humanoid animation (`Humanoid::Animate`)

```
walkPhase += speed · dt · 4              advances with distance → feet do not slide
moveBlend → 1 while moving, → 0 when stopped:   moveBlend += (target − moveBlend) · min(1, 8·dt)
swing = sin(walkPhase) · 35° · moveBlend

LeftHip.rotX  = +swing      RightHip.rotX  = −swing          legs in opposite phase
LeftShoulder  = −0.8·swing  RightShoulder  = +0.8·swing      arms opposite to the legs
Pelvis.y      = 0.85 + |sin(walkPhase)|·0.04·moveBlend        body bob (twice per stride)
Head.rotY     = sin(0.6·t)·25°·(1 − moveBlend)               idle look-around
Seated pose   : hips (−40°, 0, ±32°) astride, shoulders −50° (hands on the reins), blended by seatBlend
```

Buzz adds a separate **flight rig** (blended in with `flightBlend`): both legs trail together, the body
pitches forward by `6° + 44°·cruise − climbTilt` (cruise = horizontal speed / 1.8), banks by
`−turnRate·0.12` (clamped to ±14°) and the walking gait is switched off (`groundMotion = 0`).

## 3. Bullseye (`src/characters/Bullseye.cpp`)

```
Root (hooves on floor, heading)
└─ Body                      joint    y bob = |cos(1.2·phase)|·0.06 while galloping
   ├─ Barrel                 sphere   pos (0, 1.25, 0)    size (0.72, 0.66, 1.50)   ellipsoid, woven texture ×3
   ├─ Belly                  sphere   pos (0, 1.08, 0)    size (0.50, 0.30, 1.00)   lighter colour
   ├─ Saddle                 joint    pos (0, 1.55, 0)
   │  ├─ SaddlePad           cube     size (0.62, 0.10, 0.62)                       leather texture
   │  ├─ SaddleFlapL/R       cube     pos (±0.33, −0.18, 0) size (0.05, 0.35, 0.40)
   │  ├─ Horn                cyl      pos (0, 0.10, 0.26) size (0.07, 0.16, 0.07)   brass
   │  ├─ StirrupStrap ×2     cube     pos (±0.36, −0.30, 0.12) size (0.035, 0.52, 0.035)
   │  ├─ StirrupBase ×2      cube     pos (±0.36, −0.57, 0.12) size (0.16, 0.025, 0.13)   brass
   │  └─ Seat                joint    pos (0, 0.06, −0.05)      ← Jessie attaches here
   ├─ Neck                   joint    pos (0, 1.42, 0.62) rot (35, 0, 0)   leans forward, nods while galloping
   │  ├─ NeckShape           cyl      pos (0, 0.38, 0)    size (0.30, 0.80, 0.34)
   │  ├─ Mane                cube     pos (0, 0.42, −0.16) size (0.07, 0.85, 0.12)  woven texture ×(1, 4)
   │  └─ Head                joint    pos (0, 0.80, 0) rot (−35, 0, 0)   cancels the neck tilt → level head
   │     ├─ Skull            cube     pos (0, 0.05, 0.20)  size (0.32, 0.32, 0.55)
   │     ├─ Muzzle           cube     pos (0, −0.02, 0.50) size (0.28, 0.26, 0.26)
   │     ├─ NostrilL/R       sphere   pos (±0.07, 0, 0.63) size 0.05
   │     ├─ EyeL/R, PupilL/R sphere   pos (±0.165…0.18, 0.12, 0.25…0.27)
   │     ├─ EarL/R           cone     pos (±0.10, 0.30, 0.02) size (0.10, 0.22, 0.08)
   │     ├─ Forelock         cube     pos (0, 0.23, 0.12)
   │     ├─ BridleNose       cube     pos (0, 0.02, 0.52)  size (0.30, 0.045, 0.27)  leather
   │     └─ CheekStrap ×2                    leather straps
   ├─ Tail                   joint    pos (0, 1.42, −0.72) rot (−30, 0, 0)   swish (rotZ) when idle
   │  ├─ TailShape           cyl      pos (0, −0.35, 0)   size (0.10, 0.70, 0.10)
   │  └─ TailTip             cone     pos (0, −0.80, 0)   size (0.18, 0.30, 0.18) rot (180, 0, 0)
   └─ FrontLeftHip / FrontRightHip / BackLeftHip / BackRightHip   joints at (±0.22, 1.0, ±0.5)
      ├─ Leg                 cyl      pos (0, −0.45, 0)   size (0.15, 0.90, 0.15)
      ├─ Knee                sphere   pos (0, −0.47, 0.025) size (0.18, 0.16, 0.18)
      ├─ Fetlock             sphere   pos (0, −0.82, 0)   size (0.18, 0.14, 0.18)   pale
      └─ Hoof                cyl      pos (0, −0.95, 0)   size (0.18, 0.10, 0.18)
```

Gallop: diagonal pairs move together — `FL = BR = sin(1.2·phase)·32°·moveBlend`, `FR = BL = −FL`.
Controls: W/S, A/D, Shift gallop faster, SPACE stop. Faster (2.6 u/s) than the humanoids (1.8 u/s).

## 4. RC Car (`src/characters/RCCar.cpp`)

```
Root (heading)
├─ Chassis           Body cube (0.8, 0.3, 1.4) · Cabin cube (0.7, 0.28, 0.7)
│                    Windshield, RearWindow, SideWindowL/R (glass cubes) · BumperF/B
│                    SpoilerPostL/R + Spoiler · Antenna + AntennaTip
│                    Grille (one dark block, replaces 7 bars) · HeadlightBulbL/R (emissive when on)
│                    HeadlightL / HeadlightR anchors → two spot lights
└─ WheelFL/FR/BL/BR  joints at (±0.45, 0.22, ±0.45); front ones steer (rotY = steering)
   └─ Spin           joint, rotX = wheelAngle
      ├─ Tyre        cylinder rot (0, 0, 90) size (0.44, 0.20, 0.44)   twill texture ×(6,1): the tread grain
      ├─ Hub         cylinder rot (0, 0, 90) size (0.2, 0.22, 0.2)
      ├─ Spoke       cube (0.23, 0.05, 0.36)
      └─ RadialSpoke cube rot (90, 0, 0): a cross, so the rolling stays visible
```

* **Rolling without slipping:** the wheel turns through the arc length it travels,
  `wheelAngle += degrees(speed · dt / radius)` with radius 0.22 (one metre of travel = 4.55 rad = 260°).
* **Car steering:** a car can only turn while moving — `TurnFactor = clamp(speed / maxSpeed) · 1.3`,
  so turning reverses when reversing. Front wheels show the steering angle (±28°).
* **L** toggles the headlights (two spot lights with 12°/22° cones).
* The car's wheels used to carry 72 tread cubes and 16 radial spokes; the tyre texture replaced them, and
  the second round removed the painted-on stripes and door handles (32 shapes now).

## 5. Character movement mathematics (`Character::Drive`)

All motion is multiplied by the frame time `dt`, so it is frame-rate independent.

```
heading h (degrees, 0 = facing +Z) ;   forward = ( sin h , 0 , cos h )
target speed  v* = forward-input · maxSpeed · (run ? 2 : 1)
speed         v  += clamp( v* − v , −a·dt , +a·dt )              constant acceleration a (8 u/s²)
heading       h  += turn-input · turnRate · TurnFactor · dt
position      p  += forward · v · dt
```

* Autopilot (`SteerTowards`): desired heading `atan2(Δx, Δz)`, error `Δh = wrap(desired − h)`,
  `turn = clamp(Δh / 25°, −1, 1)`, forward speed reduced to 0.35 when `|Δh| > 70°` so the toy turns first
  and walks second.
* Waypoints (`FollowWaypoint`): `step = min(distance, speed·dt)`, `p += Δ · step/distance`; the animation
  speed is set to `step/dt`, so the walk cycle matches the story's speed.
* Mount/dismount transitions use smoothstep easing `s = t²(3 − 2t)` with a hop arc `0.4·sin(πt)` added to y.
* The collision solver ([15](15-physics-and-interface.md)) then keeps every actor out of furniture and
  inside the room.

## 6. Props (`src/world/Room.cpp`)

| Prop | Construction | Behaviour |
|---|---|---|
| Floor | plane 20 × 18, wood-plank texture ×(8, 4), reflectivity 0.18 | mirror-like reflections in ray tracing |
| Walls | left wall, front wall, right wall in two pieces around the doorway, back wall in four pieces around the window opening; wallpaper texture, tiled once per ~2 units | one-sided (doll's house view) |
| Ceiling | plane rotated 180° about X | |
| Doorway + hall | lintel plane, two jambs, header, hall floor, two side walls, end wall, hall ceiling; a night-light point lamp at (13.5, 3.7, 4) | connects to the story's outside area |
| Window | 4 frame cubes + cross mullions + sill, opening x ∈ [0.5, 4.5], y ∈ [2.5, 5.5] | shows the sky outside |
| Sky | huge plane 40 units behind the window, unlit, star texture | colour follows the day/night cycle |
| Sun / Moon | unlit spheres on an arc behind the window; the moon has the cratered texture | positions drive the directional light |
| Distant houses | 9 dark unlit cubes behind the sky | skyline silhouette |
| Rug | plane 6 × 4.5, procedural rug texture | |
| Poster | plane on the clear left-wall section at (-9.98, 3.8, 2.2), texture loaded from `assets/textures/poster.bmp` by our BMP loader | Positioned beside the bookcase so the full printed face remains readable |
| Toy blocks | a 6-cube tower plus the mission's doorway crate (1.2 × 2 × 3) | gravity, collisions; Buzz's laser pushes them |
| Desk | top + 4 legs + drawer cubes, knob sphere, sketchbook, 3 pencils | solid furniture |
| Bed | frame, mattress, quilted blanket (fabric texture), headboard, 2 pillows | solid furniture |
| Bookcase | 2 uprights, back, 4 shelves, and on each shelf one box showing a row of book spines (texture) | solid furniture |
| Curtains | 2 × 4 fabric-textured folds + a rod | |
| Trim | skirting boards and crown moulding | |
| Desk lamp (key 7) | Base cyl → ArmJoint (tilt) → Arm cyl + Elbow sphere → HeadJoint (tilt) → Shade cone, Bulb sphere (emissive), LightAnchor | point + spot light, flickers and looks around at night; A/D swivel, W/S tilt, R power |
| Ball (key 6) | root → BallShape sphere Ø 0.7 with beach-ball texture | rolls by itself at night; W/S/A/D push relative to the camera, bounces off walls |
| Ghost (key 8) | Body joint → Head sphere, Sheet cone, eyes and mouth | transparent, fades in at night, circles the room, bluish point light |
| Contact shadows | a flattened translucent dark sphere under each toy | soft blob shadow where the toy meets the floor (raster mode, lighting + shading on) |

Around the room stands the **house** (facade, roof, porch, garage, garden, ground-floor corridor, stairs,
three doors), and **Penny the cat** walks into it before the story: both are described in
[18 — The house, Penny the cat and the arrival](18-house-and-penny.md). The bed has the orange plaid sheet
and beige pillows from the photos of Penny.

## 7. Selection summary

| Key | Object | Controls |
|---|---|---|
| 1 | Woody | W/S move, A/D turn, Shift run, SPACE stop |
| 2 | Jessie | same + R mount/dismount Bullseye (when mounted, W/S/A/D drive Bullseye) |
| 3 | Bullseye | same + R mount/dismount |
| 4 | Buzz | same + Q/E fly, L laser |
| 5 | RC Car | same + L headlights |
| 6 | Ball | W/S/A/D push, SPACE stop |
| 7 | Desk lamp | A/D swivel, W/S tilt, R power, , / . brightness |
| 8 | Ghost | (inspect / edit mode) |
| 0 | nothing | camera only |
| (click) | Penny | inspect only (F orbit, V parts, edit mode); she watches the story from the bed |
