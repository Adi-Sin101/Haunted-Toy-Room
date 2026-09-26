# 04 — 3D Transformations (Lecture 3)

All transformation matrices used by the project are written **by hand, entry by entry** in
`src/math/Transform3D.cpp` (namespace `t3d`). GLM is used only as the storage type (`glm::mat4`) and for
matrix multiplication / inversion.

## 1. Homogeneous coordinates

A 3D point is written as a 4-vector `(x, y, z, 1)`, a direction as `(x, y, z, 0)`. With the extra
coordinate, **translation becomes a matrix multiplication** like rotation and scaling, so any sequence of
transformations collapses into one 4×4 matrix:

```
| x' |   | m00 m01 m02 m03 |   | x |
| y' | = | m10 m11 m12 m13 | · | y |        p' = M · p   (column vector convention)
| z' |   | m20 m21 m22 m23 |   | z |
| 1  |   |  0   0   0   1  |   | 1 |
```

Directions (w = 0) ignore the translation column — exactly right for normals and velocities.

### Row-major on paper, column-major in memory
The lecture writes matrices row by row. GLM (and OpenGL) store them **column by column**: `m[col][row]`.
`t3d::fromRows(...)` takes the 16 numbers in lecture (row) order and transposes them into GLM's layout, so
every function below reads exactly like the slide.

## 2. Basic transformations

### Translation `t3d::translate(t)`
```
| 1 0 0 tx |
| 0 1 0 ty |        x' = x + tx,  y' = y + ty,  z' = z + tz
| 0 0 1 tz |
| 0 0 0 1  |
```

### Scaling `t3d::scale(s)` (about the origin)
```
| sx 0  0  0 |
| 0  sy 0  0 |      x' = sx·x, ...     sx = −1 would mirror
| 0  0  sz 0 |
| 0  0  0  1 |
```

### Rotation about the coordinate axes (right-handed, positive = counter-clockwise looking down the axis)
```
Rx(θ) = | 1  0    0   0 |   Ry(θ) = |  cos 0 sin 0 |   Rz(θ) = | cos −sin 0 0 |
        | 0 cos −sin  0 |           |  0   1  0  0 |           | sin  cos 0 0 |
        | 0 sin  cos  0 |           | −sin 0 cos 0 |           | 0    0   1 0 |
        | 0  0    0   1 |           |  0   0  0  1 |           | 0    0   0 1 |
```
Note the sign pattern of `Ry`: it follows from the cyclic order x → y → z → x (rotating z toward x).

### Rotation about an arbitrary axis through the origin `t3d::rotateAxis(u, θ)` — Rodrigues
For a unit axis `u = (x, y, z)`, `c = cos θ`, `s = sin θ`, `t = 1 − c`:
```
| t·x·x + c    t·x·y − s·z  t·x·z + s·y  0 |
| t·x·y + s·z  t·y·y + c    t·y·z − s·x  0 |
| t·x·z − s·y  t·y·z + s·x  t·z·z + c    0 |
| 0            0            0            1 |
```
Used by the rolling ball: axis = `up × velocity`, angle = distance / radius.

### Shear `t3d::shear(xy, xz, yx, yz, zx, zy)`
```
| 1  xy xz 0 |     x' = x + xy·y + xz·z
| yx 1  yz 0 |     y' = y + yx·x + yz·z
| zx zy 1  0 |     z' = z + zx·x + zy·y
| 0  0  0  1 |
```
Try it: select any object, **Tab** (edit mode), **T** until "Shear", then J/L, U/O, I/K.

### Reflection `t3d::reflect(n)` — about the plane through the origin with unit normal n
```
M = I − 2·n·nᵀ   →   | 1−2nx²   −2nx·ny  −2nx·nz  0 |
                     | −2ny·nx  1−2ny²   −2ny·nz  0 |
                     | −2nz·nx  −2nz·ny  1−2nz²   0 |
                     | 0        0        0        1 |
```
For n = (1, 0, 0) this is `scale(−1, 1, 1)`: a mirror in the YZ plane. **M** in edit mode applies it.
A reflection flips the triangle winding (determinant −1); the lighting still works because normals are
transformed with the normal matrix (section 5).

## 3. Composite transformations

Matrices apply **right to left**: in `A · B · p`, B acts first.

### About a fixed point `t3d::scaleAboutPoint(s, p)`
Move p to the origin, scale, move back:  `T(p) · S(s) · T(−p)`.

### About an arbitrary axis through point p `t3d::rotateAboutAxis(p, u, θ)`
`T(p) · R(u, θ) · T(−p)`.

In the scene graph we rarely need these explicitly: a **joint node** placed at the pivot does the same
job (see [06](06-scene-graph-hierarchy.md)). A shoulder joint sits at the shoulder; rotating the joint
rotates the arm about the shoulder, not about the arm's centre.

### Order matters
`T · R` (rotate, then translate) spins the object in place and then moves it.
`R · T` (translate, then rotate) swings it around the origin like a planet.

## 4. The transform of every scene node

`src/scene/Transform.h`:

```
M_local = T(position) · Ry(yaw) · Rx(pitch) · Rz(roll) · B · S(scale)
```

Read right to left: scale the unit shape to size → optional extra linear part `B` (shear / reflection /
accumulated rolling rotation) → roll → pitch → yaw → move into place. Keeping position / Euler angles /
scale as separate numbers makes them directly editable by the controls and the inspector, while the matrix
is rebuilt from them whenever needed.

| Field | Meaning | Who changes it |
|---|---|---|
| `position` | translation | W/S driving, edit mode translate, story autopilot |
| `rotation.y` (yaw) | heading of a character | A/D, edit mode rotate |
| `rotation.x` / `.z` | pitch / roll | limb animation, lamp tilt, edit mode rotate |
| `scale` | size | edit mode scale, shape sizes, Buzz's wings opening |
| `basis` | extra 3×3 linear part | ball rolling, edit mode shear / mirror |

## 5. Transforming normals — the normal matrix

Positions transform with M, but normals must stay **perpendicular** to the surface. Under non-uniform scale
or shear, `M · n` is no longer perpendicular. The correct transform is

```
N = (M₃ₓ₃⁻¹)ᵀ          t3d::normalMatrix(model)
```

Proof sketch: a tangent `t` on the surface satisfies `n·t = 0`. After transformation `t' = M t`; we need
`n'·t' = 0` → `(N n)ᵀ (M t) = nᵀ Nᵀ M t = 0` for all t → `Nᵀ M = I` → `N = (M⁻¹)ᵀ`.

It is computed on the CPU once per draw item (`Renderer::CollectNode`) and sent as `uNormalMatrix`.

## 6. Coordinate spaces used in the project

```
object space ──model (M)──► world space ──view (V)──► camera space ──projection (P)──► clip space
 (unit cube)     scene graph   (the room)   lookAt       (camera at 0,     perspective   ──÷w──► NDC ──► pixels
                                                          looking −Z)
```

* **Object / local space** — the unit primitive, or a node relative to its parent.
* **World space** — the room: floor at y = 0, x ∈ [−8, 8], z ∈ [−6, 6], height 7. Lighting is done here.
* **Camera space**, **clip space**, **NDC**, **window** — see [05 — Camera](05-camera.md).

## 7. Live demonstration checklist (edit mode)

1. Select an object (1–8 or click). **Tab** → edit mode, title bar shows `[EDIT: Translate]`.
2. **J/L**, **U/O**, **I/K** move along X, Y, Z. The coloured gizmo (X red, Y green, Z blue) follows.
3. **T** → Rotate: I/K pitch, J/L yaw, U/O roll. The gizmo axes rotate — they *are* the columns of the
   world matrix.
4. **T** → Scale: per-axis; hold **Ctrl** for uniform scale.
5. **T** → Shear: the gizmo axes stop being perpendicular.
6. **M** mirrors. **Backspace** restores the original transform. **Shift** makes every step 3× faster.
7. **V** prints every part's local position/size and world position — local numbers stay the same while
   world numbers change as the parent moves.
