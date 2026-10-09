# 05 — Camera, Viewing and Projection

Files: `src/scene/Camera.*`, `src/math/Transform3D.cpp` (`lookAt`, `perspective`),
`ToyRoomApp::HandleCamera`.

## 1. Camera description

The camera is a position plus two angles:

```
yaw   (degrees, 0 = looking down −Z, positive turns right)
pitch (degrees, positive looks up, clamped to ±89°)

forward = ( cos(pitch)·sin(yaw),  sin(pitch),  −cos(pitch)·cos(yaw) )
right   = normalize( forward × worldUp )
up      = right × forward
```

`fov` is the vertical field of view in degrees — changing it is an **optical zoom**.

## 2. View matrix — `t3d::lookAt(eye, target, up)`

The view matrix moves the whole world so that the camera sits at the origin looking down −Z:

```
f = normalize(target − eye)          (forward)
r = normalize(f × up)                (right)
u = r × f                            (true up)

      |  r.x   r.y   r.z   −r·eye |
V  =  |  u.x   u.y   u.z   −u·eye |
      | −f.x  −f.y  −f.z    f·eye |
      |  0     0     0      1     |
```

The upper 3×3 is a rotation whose rows are the camera axes (it expresses a world vector in camera
coordinates); the last column is that rotation applied to −eye (translate the camera to the origin first).

## 3. Projection matrix — `t3d::perspective(fovy, aspect, near, far)`

```
t = tan(fovy / 2)

      | 1/(aspect·t)  0     0                 0               |
P  =  | 0             1/t   0                 0               |
      | 0             0    −(f+n)/(f−n)      −2fn/(f−n)       |
      | 0             0    −1                 0               |
```

* The `−1` in the last row puts `−z_camera` into `w`; the GPU's division by `w` makes distant things
  smaller — that is perspective.
* x and y are scaled so the view frustum's edges land on NDC ±1.
* z is mapped non-linearly so near → −1 and far → +1 (used by the depth test).
* near = 0.05, far = 200 (`Camera::nearPlane / farPlane`).

The final vertex position is `gl_Position = P · V · M · p`.

## 4. Camera modes (C cycles)

| Mode | Purpose | Controls |
|---|---|---|
| **Free** | fly anywhere | arrows move forward/back/strafe, PgUp/PgDn up/down, right-drag look, middle-drag pan, scroll = zoom (FOV 10°–90°), Shift = 3× speed |
| **Orbit** | inspect the selected object closely | right-drag or arrows orbit, scroll or PgUp/PgDn change distance (0.6–60 units), middle-drag pans the target |
| **Follow** | third-person chase camera | stays behind the driven character, scroll = distance |

**F** — focus: switches to Orbit around the selected object at a distance chosen for that object.
**Numpad 1 / 3 / 7** (or **Ctrl + 1 / 3 / 7**) — front / side / top views of the current orbit target.
**Home** — reset to the starting view.

### Orbit maths (`Camera::ApplyOrbit`)
The camera lies on a sphere around the target:

```
offset   = ( cos(pitch)·sin(yaw),  sin(pitch),  cos(pitch)·cos(yaw) )
position = target + offset · distance
then LookAt(target)   →  yaw = atan2(d.x, −d.z),  pitch = asin(d.y)   with d = normalize(target − position)
```

The orbit target glides toward the selected object each frame (`mix` with a rate × dt), so the camera keeps
following a walking character smoothly.

### Follow maths
`desired = focus − forward·distance + up·(0.45·distance)`. Everything is eased with frame-rate independent
exponential smoothing, `x += (target − x)(1 − e^(−k·dt))`:

| Quantity | Rate k | Why |
|---|---|---|
| focus x, z | 14 | follows the character closely across the floor |
| focus y | 5 | stair and porch treads lift the character 0.25 at a time; aiming at the raw height jolted the view on every step |
| camera position | 4 | the chase lag |
| wall reach (out) | 2.5 | a wall pulls the camera in **at once** but releases it gradually, so railings that block and clear the view on alternate frames no longer pump it |

A focus jump of more than 2.5 units (new selection, restart) snaps. In the upstairs hallway the camera
changes sides only when the other side is at least 0.6 units roomier. On the scripted story the pitch
acceleration on stairs dropped from 2.42 to 0.14 °/frame² RMS, with 1 jolt above 0.5 instead of 82
([21](21-outdoor-sky-and-solid-characters.md)).

## 5. Mouse picking — the inverse of projection

Clicking selects the object under the cursor (`ToyRoomApp::Pick`):

1. Pixel → NDC: `x = 2·mx/width − 1`, `y = 1 − 2·my/height` (window y grows downward).
2. NDC → world direction:
   `dir = normalize( forward + x·tan(fov/2)·aspect·right + y·tan(fov/2)·up )`.
3. For every draw item: transform the ray into the item's object space with the inverse model matrix and
   intersect the exact unit primitive (`RayIntersect::Object`, same maths as the ray tracer).
4. The nearest hit's owner becomes the selection.

A left-click only picks if the mouse moved less than 4 pixels while pressed, so dragging does not select.

## 6. Why the room looks like a doll's house from outside

The walls are one-sided planes facing inward. From the default camera (outside the front wall) the front
wall's triangles are back-facing → culled → the room is open toward the viewer. Walk the free camera inside
the room (arrow keys) and all four walls surround you.
