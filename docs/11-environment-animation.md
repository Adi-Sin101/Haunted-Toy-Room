# 11 — Environment Motion, Animation and the Story

Files: `src/world/Environment.*`, `src/world/StoryDirector.*`, `src/characters/*`.

The room moves on its own even when nobody touches the keyboard. Every motion is multiplied by the frame's
delta time `dt`, so speeds are in units per **second**, independent of the frame rate.

![Night story](images/night-story.png)

*Environment animation reference. Default playback now follows The Midnight Mission described in section 6.*

## 1. The clock and the day/night cycle

* `hour` runs 0 … 24. One full day lasts `dayLengthSeconds = 150` real seconds × `timeScale`.
* Keys: **P** pause, **[** / **]** halve / double the speed (0.25× … 32×), **,** / **.** scrub backward /
  forward.
* Penny's arrival runs the clock from 16:18 to midnight while she walks into the house
  ([18](18-house-and-penny.md)); the mission then starts at midnight and advances to 08:00 during its
  Morning scene.

**Sun and moon.** Angle `a = (hour − 6) / 12 · π` (0 at 06:00, π at 18:00). Both bodies move on a half
ellipse behind the window:

```
position(angle) = skyCentre + ( −cos(angle)·3.2,  sin(angle)·3.1,  0 )
sun  = position(a)          moon = position(a + π)
```

From the garden (during the arrival) the sun is drawn on a much larger arc in front of the sky backdrop,
`OutdoorSunPosition = (−60 cos a, 4 + 55 sin a, −37)`, so it can be seen above the house's roof.
The neighbouring houses' silhouettes are recoloured with the daylight
(`mix((0.035, 0.045, 0.085), (0.50, 0.52, 0.60), daylight)`).

The directional light points from the visible body to the room centre. `daylight =
smoothstep(−0.1, 0.25, sin a)` blends ambient light, clear colour, sky colour and star visibility; an extra
orange term `exp(−6·|sin a|)` tints the sky at sunrise and sunset.

## 2. Haunted desk lamp

* Switches **on** at night, off by day (unless the user toggled it with R — then it stays manual).
* **Flicker:** intensity × (0.8 + 0.2·noise(9t)); when a slower noise exceeds 0.78 it drops to 15 % for a
  moment — the "haunted" stutter. The bulb's emissive colour flickers with the light.
* **Looking around:** at night the lamp swivels ±35° and nods ±12° with slow sine waves.
  When the lamp is selected, A/D and W/S drive these angles by hand instead.

## 3. Rolling ball

* At night (when not selected) it follows a Lissajous path
  `(−2 + 3 sin s, r, 2.5 + 1.8 sin 2s)` using velocity steering; otherwise rolling friction slows it.
* When selected: W/S/A/D push it relative to the camera (so W always rolls it away from you).
* Walls reflect the velocity component normal to the wall (with 20 % energy loss).
* **Rolling without slipping:** each frame
  ```
  axis  = normalize(up × displacement)
  angle = |displacement| / radius
  basis = rotateAxis(axis, angle) · basis        (accumulated in the shape node's basis matrix)
  ```
  so the texture turns exactly as much as the ball travels.

## 4. The ghost

Fades in at night (`opacity = 0.55 · visibility`), circles the room on an ellipse at ~4 units height with
a bobbing sine, turns to face its direction of travel (`yaw = atan2(vx, vz)`) and sways. It carries a faint
blue point light. When selected (8) it holds still so it can be inspected and edited.

## 5. Character animation

All toys share the `Character` movement code ([07](07-characters-and-props.md)):

```
targetSpeed = input.forward · maxSpeed · (Shift ? 2 : 1)
speed      += clamp(targetSpeed − speed, −accel·dt, +accel·dt)       constant acceleration
heading    += input.turn · turnRate · TurnFactor · dt
position   += (sin heading, 0, cos heading) · speed · dt
```

* **SPACE** sets speed to 0 immediately; holding it keeps the toy stopped.
* Positions are clamped to the room (a simple bound, not collision detection).
* Walk cycles, gallop, wheel roll and Buzz's wings are described in
  [07](07-characters-and-props.md). Every cycle is driven by `walkPhase`, which advances with the distance
  travelled, and `moveBlend`, which eases the pose back to neutral when the toy stops.
* **Transitions** (mount / dismount) interpolate position and heading over 0.6–0.7 s with smoothstep
  easing and a small hop arc `sin(πt)·0.4`, so nothing teleports.

## 6. Penny's arrival (prologue)

Before the story, Penny the cat walks from the street into the house, up the stairs and onto the bed, with
a chase camera, opening doors and a sunset — see [18](18-house-and-penny.md). Her trot, sitting and
sleeping poses blend with exponential easing; while she watches the story her head turns towards the toy
that is acting, and in the Morning scene she curls up asleep. **Y** skips the arrival.

## 7. The Midnight Mission (StoryDirector)

Default playback follows seven coordinated scenes: Discovery, Moving Outside, Clearing the Path,
Reaching the Car, Activating the Car, Returning Home and Morning. The right-side doorway connects
with a bounded hallway, where the lost car waits. Buzz must hit the obstruction with a real laser
impulse; arrival state gates each route. Jessie mounts Bullseye through the existing saddle hierarchy.
The activated car drives a predefined path with rotating wheels. Finally the toys return to their
saved positions and poses, the lamp turns off, dawn fades in and the camera pulls back.

Selecting a toy pauses the film for manual control. **N** resumes it; **Shift+N** restores the setup
and restarts Discovery. **P** pauses the mission. **Enter** activates the car during Scene 5;
unattended playback performs the interaction after three seconds. Camera input overrides cinematic
shots without stopping the story. **H** expands the compact corner guide; **G** hides all text.

See [the complete mission and its implementation](16-midnight-mission.md). The optional haunting
prop animations described above remain in Environment, while mission playback keeps the ball and
ghost still. The film owns the midnight-to-morning clock; clock speed and scrubbing are manual controls.
