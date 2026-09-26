# 11 — Environment Motion, Animation and the Story

Files: `src/world/Environment.*`, `src/world/StoryDirector.*`, `src/characters/*`.

The room moves on its own even when nobody touches the keyboard. Every motion is multiplied by the frame's
delta time `dt`, so speeds are in units per **second**, independent of the frame rate.

![Night story](images/night-story.png)

*23:00 with the story on: Woody and Jessie patrol, Buzz flies, the ball rolls, the ghost floats, the lamp shines on the rug.*

## 1. The clock and the day/night cycle

* `hour` runs 0 … 24. One full day lasts `dayLengthSeconds = 150` real seconds × `timeScale`.
* Keys: **P** pause, **[** / **]** halve / double the speed (0.25× … 32×), **,** / **.** scrub backward /
  forward.
* The program starts at 20:30 (dusk turning into night).

**Sun and moon.** Angle `a = (hour − 6) / 12 · π` (0 at 06:00, π at 18:00). Both bodies move on a half
ellipse behind the window:

```
position(angle) = skyCentre + ( −cos(angle)·16,  sin(angle)·9,  0 )
sun  = position(a)          moon = position(a + π)
```

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

## 6. The story (StoryDirector) — "toys come alive at night"

**N** toggles the story on/off (default on). While on:

* **Night:** every toy that is *not selected by the user* walks its own patrol loop (a list of waypoints).
  Steering: turn toward the next waypoint (`turn = clamp(Δheading / 25°)`), walk forward when roughly facing
  it, switch to the next waypoint within 0.35 units. Buzz climbs to 1.6 units and flies his loop; the car
  drives laps; Bullseye trots (carrying Jessie if she is mounted).
* **Morning:** each toy walks back to its **home** position (where the child left it), turns to its original
  heading and freezes — "as morning arrives the toys return to their original positions and become still
  again".
* The selected character is always under manual control. Select **0** to hand everyone back to the story.

A complete short movie: start the program, press **]** a couple of times, and watch dusk → haunted night
(lamp flickers, ghost appears, toys wander, ball rolls) → sunrise (everyone returns home) → day.
