# 03 — Primitives: Vertices, Indices, Triangles

Every object in the scene — room, furniture, five characters, ghost — is assembled from **five
primitive meshes** whose vertices and indices we write ourselves in `src/geometry/Primitives.cpp`.
They are uploaded to the GPU **once** (`Assets::Load`) and shared: every sphere in the scene (heads,
eyes, hands, the ball, the sun…) is drawn from the same sphere VBO with a different model matrix.

| Primitive | Unit shape (object space) | Vertices | Triangles |
|---|---|---|---|
| Plane | 1 × 1 square in XZ at y = 0, facing +Y | 4 | 2 |
| Cube | [−0.5, 0.5]³ | 24 | 12 |
| Sphere | radius 0.5 (18 stacks × 32 sectors) | 627 | 1088 |
| Cylinder | radius 0.5, y ∈ [−0.5, 0.5], capped (32 sectors) | 134 | 128 |
| Cone | base radius 0.5 at y = −0.5, apex at y = +0.5, capped (32 sectors) | 99 | 64 |

**Why unit size?** A model matrix scale of `(w, h, d)` then produces an object of exactly `w × h × d`
units, so the numbers in the character builders read like real dimensions ("a leg 0.15 wide and 0.62
long"). It also lets the ray tracer intersect the exact analytic unit shape in object space
([12 — Ray tracing](12-ray-tracing.md)).

**Rule for every triangle:** its three vertices are listed **counter-clockwise when seen from outside**.
That is OpenGL's front face, so back-face culling removes only hidden faces. How to check it: pick a viewer
outside the face, write down the screen positions (right, up) of the three vertices a → b → c, and compute
the 2D cross product `(b−a) × (c−a)`; positive = counter-clockwise.

Each vertex stores `position`, `normal`, `uv` (see [02](02-opengl-pipeline-fundamentals.md)).

You can print the real tables from the running program: select an object and press **Shift+V** — every
primitive it uses is dumped with all vertices and all triangles. **F10** draws the vertices of the selected
object as points, **F9** draws its normals, **F1** shows all triangles as wireframe.

---

## 1. Plane — 4 vertices, 2 triangles

```
   v3 (-0.5, 0, -0.5) ─────────── v2 (0.5, 0, -0.5)          view from above (+Y),
          │                  ╱          │                      screen up = -Z
          │     T1       ╱              │
          │          ╱        T0        │
          │      ╱                      │
   v0 (-0.5, 0,  0.5) ─────────── v1 (0.5, 0, 0.5)
```

| idx | position | normal | uv |
|---|---|---|---|
| 0 | (−0.5, 0, 0.5) | (0, 1, 0) | (0, 0) |
| 1 | (0.5, 0, 0.5) | (0, 1, 0) | (1, 0) |
| 2 | (0.5, 0, −0.5) | (0, 1, 0) | (1, 1) |
| 3 | (−0.5, 0, −0.5) | (0, 1, 0) | (0, 1) |

Indices: `T0 = 0, 1, 2` and `T1 = 2, 3, 0`.

Winding check for T0 seen from above (screen x = world x, screen y = −z):
a = (−0.5, −0.5), b = (0.5, −0.5), c = (0.5, 0.5) → (b−a) × (c−a) = (1, 0) × (1, 1) = 1·1 − 0·1 = **+1 → CCW** ✓.

Used for: floor, ceiling, walls, rug, poster, sky backdrop.

## 2. Cube — 24 vertices, 12 triangles, 36 indices

A cube has 8 corners, but a vertex can hold only **one** normal and **one** uv. Each corner belongs to three
faces that point in three different directions, so each corner is stored three times — once per face —
giving 6 faces × 4 = 24 vertices. (With 8 shared vertices the normals would have to be averaged and the
cube would be lit like a blob; flat faces need their own normals.)

Each face lists its corners in the order bottom-left, bottom-right, top-right, top-left **as seen by a viewer
standing outside that face**, with uv (0,0), (1,0), (1,1), (0,1). Then the face is two triangles
`(b, b+1, b+2)` and `(b+2, b+3, b)` where `b` is the face's first vertex index.

| idx | face | position | normal | uv |
|---|---|---|---|---|
| 0 | Front +Z | (−0.5, −0.5, 0.5) | (0, 0, 1) | (0, 0) |
| 1 | Front +Z | (0.5, −0.5, 0.5) | (0, 0, 1) | (1, 0) |
| 2 | Front +Z | (0.5, 0.5, 0.5) | (0, 0, 1) | (1, 1) |
| 3 | Front +Z | (−0.5, 0.5, 0.5) | (0, 0, 1) | (0, 1) |
| 4 | Back −Z | (0.5, −0.5, −0.5) | (0, 0, −1) | (0, 0) |
| 5 | Back −Z | (−0.5, −0.5, −0.5) | (0, 0, −1) | (1, 0) |
| 6 | Back −Z | (−0.5, 0.5, −0.5) | (0, 0, −1) | (1, 1) |
| 7 | Back −Z | (0.5, 0.5, −0.5) | (0, 0, −1) | (0, 1) |
| 8 | Right +X | (0.5, −0.5, 0.5) | (1, 0, 0) | (0, 0) |
| 9 | Right +X | (0.5, −0.5, −0.5) | (1, 0, 0) | (1, 0) |
| 10 | Right +X | (0.5, 0.5, −0.5) | (1, 0, 0) | (1, 1) |
| 11 | Right +X | (0.5, 0.5, 0.5) | (1, 0, 0) | (0, 1) |
| 12 | Left −X | (−0.5, −0.5, −0.5) | (−1, 0, 0) | (0, 0) |
| 13 | Left −X | (−0.5, −0.5, 0.5) | (−1, 0, 0) | (1, 0) |
| 14 | Left −X | (−0.5, 0.5, 0.5) | (−1, 0, 0) | (1, 1) |
| 15 | Left −X | (−0.5, 0.5, −0.5) | (−1, 0, 0) | (0, 1) |
| 16 | Top +Y | (−0.5, 0.5, 0.5) | (0, 1, 0) | (0, 0) |
| 17 | Top +Y | (0.5, 0.5, 0.5) | (0, 1, 0) | (1, 0) |
| 18 | Top +Y | (0.5, 0.5, −0.5) | (0, 1, 0) | (1, 1) |
| 19 | Top +Y | (−0.5, 0.5, −0.5) | (0, 1, 0) | (0, 1) |
| 20 | Bottom −Y | (−0.5, −0.5, −0.5) | (0, −1, 0) | (0, 0) |
| 21 | Bottom −Y | (0.5, −0.5, −0.5) | (0, −1, 0) | (1, 0) |
| 22 | Bottom −Y | (0.5, −0.5, 0.5) | (0, −1, 0) | (1, 1) |
| 23 | Bottom −Y | (−0.5, −0.5, 0.5) | (0, −1, 0) | (0, 1) |

| Face | Triangles (indices) |
|---|---|
| Front | 0 1 2 · 2 3 0 |
| Back | 4 5 6 · 6 7 4 |
| Right | 8 9 10 · 10 11 8 |
| Left | 12 13 14 · 14 15 12 |
| Top | 16 17 18 · 18 19 16 |
| Bottom | 20 21 22 · 22 23 20 |

**How "seen from outside" was chosen for each face.** For a viewer looking along direction `f` with
right vector `r`, the screen-up vector is `u = r × f`:

| Face | viewer looks along f | right r | up u = r × f |
|---|---|---|---|
| Front +Z | −Z | +X | +Y |
| Back −Z | +Z | −X | +Y |
| Right +X | −X | −Z | +Y |
| Left −X | +X | +Z | +Y |
| Top +Y | −Y | +X | −Z |
| Bottom −Y | +Y | +X | +Z |

The four corners are then (−r −u), (+r −u), (+r +u), (−r +u) — bottom-left, bottom-right, top-right,
top-left — which is always counter-clockwise for that viewer.

Used for: bodies, torsos, belts, feet, desk, window frame, toy blocks, car body, horse head…

## 3. Sphere — latitude / longitude grid

Parameters: `stacks` (horizontal rings, pole to pole) and `sectors` (vertical slices around).

For ring `i = 0 … stacks` and column `j = 0 … sectors`:

```
phi   = 90° − i · 180° / stacks        latitude  (+90° = north pole … −90° = south pole)
theta = j · 360° / sectors             longitude

x = r · cos(phi) · sin(theta)
y = r · sin(phi)
z = r · cos(phi) · cos(theta)          with r = 0.5

normal = position / r                  (a sphere's normal points straight away from its centre)
uv     = ( j / sectors ,  1 − i / stacks )
```

Column `j = sectors` repeats column 0 at θ = 360° but with `u = 1` instead of `u = 0`, so the texture
wraps without a visible seam. Vertex count = `(stacks + 1) · (sectors + 1)`.

**Indices.** For the quad between ring `i` (upper, first index `k1 = i·(sectors+1) + j`) and ring `i+1`
(lower, `k2 = k1 + sectors + 1`):

```
 k1 ────── k1+1          T0 = (k1, k2, k1+1)       skipped when i = 0          (top ring is a single point)
  │      ╱  │            T1 = (k1+1, k2, k2+1)     skipped when i = stacks−1   (bottom ring is a single point)
  │    ╱    │
 k2 ────── k2+1
```

Triangle count = `2 · sectors · (stacks − 1)`.

Winding check for T0 seen from outside at θ ≈ 0 (the +Z side: screen right = +X, up = +Y):
k1 is top-left, k2 bottom-left, k1+1 top-right → (0,1), (0,0), (1,1) → (0,−1) × (1,0) = 0·0 − (−1)·1 = **+1 → CCW** ✓.

### Worked example: stacks = 4, sectors = 4 (25 vertices, 24 triangles)

| idx | ring i | col j | φ | θ | position (x, y, z) | uv |
|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 90 | 0 | (0.000, 0.500, 0.000) | (0.00, 1.00) |
| 1 | 0 | 1 | 90 | 90 | (0.000, 0.500, 0.000) | (0.25, 1.00) |
| 2 | 0 | 2 | 90 | 180 | (0.000, 0.500, 0.000) | (0.50, 1.00) |
| 3 | 0 | 3 | 90 | 270 | (0.000, 0.500, 0.000) | (0.75, 1.00) |
| 4 | 0 | 4 | 90 | 360 | (0.000, 0.500, 0.000) | (1.00, 1.00) |
| 5 | 1 | 0 | 45 | 0 | (0.000, 0.354, 0.354) | (0.00, 0.75) |
| 6 | 1 | 1 | 45 | 90 | (0.354, 0.354, 0.000) | (0.25, 0.75) |
| 7 | 1 | 2 | 45 | 180 | (0.000, 0.354, −0.354) | (0.50, 0.75) |
| 8 | 1 | 3 | 45 | 270 | (−0.354, 0.354, 0.000) | (0.75, 0.75) |
| 9 | 1 | 4 | 45 | 360 | (0.000, 0.354, 0.354) | (1.00, 0.75) |
| 10 | 2 | 0 | 0 | 0 | (0.000, 0.000, 0.500) | (0.00, 0.50) |
| 11 | 2 | 1 | 0 | 90 | (0.500, 0.000, 0.000) | (0.25, 0.50) |
| 12 | 2 | 2 | 0 | 180 | (0.000, 0.000, −0.500) | (0.50, 0.50) |
| 13 | 2 | 3 | 0 | 270 | (−0.500, 0.000, 0.000) | (0.75, 0.50) |
| 14 | 2 | 4 | 0 | 360 | (0.000, 0.000, 0.500) | (1.00, 0.50) |
| 15 | 3 | 0 | −45 | 0 | (0.000, −0.354, 0.354) | (0.00, 0.25) |
| 16 | 3 | 1 | −45 | 90 | (0.354, −0.354, 0.000) | (0.25, 0.25) |
| 17 | 3 | 2 | −45 | 180 | (0.000, −0.354, −0.354) | (0.50, 0.25) |
| 18 | 3 | 3 | −45 | 270 | (−0.354, −0.354, 0.000) | (0.75, 0.25) |
| 19 | 3 | 4 | −45 | 360 | (0.000, −0.354, 0.354) | (1.00, 0.25) |
| 20 | 4 | 0 | −90 | 0 | (0.000, −0.500, 0.000) | (0.00, 0.00) |
| 21 | 4 | 1 | −90 | 90 | (0.000, −0.500, 0.000) | (0.25, 0.00) |
| 22 | 4 | 2 | −90 | 180 | (0.000, −0.500, 0.000) | (0.50, 0.00) |
| 23 | 4 | 3 | −90 | 270 | (0.000, −0.500, 0.000) | (0.75, 0.00) |
| 24 | 4 | 4 | −90 | 360 | (0.000, −0.500, 0.000) | (1.00, 0.00) |

Triangles (24):

```
ring 0-1 (top cap, T1 only):  (1,5,6) (2,6,7) (3,7,8) (4,8,9)
ring 1-2:                     (5,10,6) (6,10,11) (6,11,7) (7,11,12) (7,12,8) (8,12,13) (8,13,9) (9,13,14)
ring 2-3:                     (10,15,11) (11,15,16) (11,16,12) (12,16,17) (12,17,13) (13,17,18) (13,18,14) (14,18,19)
ring 3-4 (bottom cap, T0):    (15,20,16) (16,21,17) (17,22,18) (18,23,19)
```

The scene uses 18 stacks × 32 sectors: 627 vertices, 1088 triangles — smooth enough for heads and the ball.

Used for: heads, eyes, pupils, noses, hands, hair, shoulder joints, horse body (scaled into an ellipsoid),
the ball, sun, moon, bulb, ghost head, Buzz's helmet.

## 4. Cylinder — side + two caps (32 sectors: 134 vertices, 128 triangles)

The side and the caps need **separate vertices** even where they touch, because the side normal points
outward horizontally while the cap normals point straight up or down.

**Side wall** — for every column `j = 0 … sectors`, θ = j·360°/sectors:

```
top    vertex 2j   : ( 0.5 sinθ,  0.5, 0.5 cosθ )   normal (sinθ, 0, cosθ)   uv (j/sectors, 1)
bottom vertex 2j+1 : ( 0.5 sinθ, −0.5, 0.5 cosθ )   normal (sinθ, 0, cosθ)   uv (j/sectors, 0)
```
Quad between column j and j+1 → triangles `(2j, 2j+1, 2j+2)` and `(2j+2, 2j+1, 2j+3)`.

**Caps** — a triangle fan: one centre vertex plus a ring of `sectors + 1` vertices with the cap normal.
Top (normal +Y): triangles `(centre, j, j+1)`. Bottom (normal −Y): `(centre, j+1, j)` — reversed,
because the bottom is seen from below. Cap uv = `(0.5 + x, 0.5 ∓ z)`, i.e. the disk mapped onto the image.

Winding check, top cap seen from above (screen x = x, screen y = −z): centre (0,0), ring j at θ = 0 →
(0, −0.5), ring j+1 at θ ≈ 90° → (0.5, 0) → (0,−0.5) × (0.5,0) = 0·0 − (−0.5)(0.5) = **+0.25 → CCW** ✓.

Counts: side 2·33 = 66 vertices, 64 triangles; caps 2·(1+33) = 68 vertices, 64 triangles.

Used for: arms, legs, necks, hat brims and crowns, lamp arm and base, horse legs and neck, wheels (rotated
90° about Z), hubs, laser beam.

## 5. Cone — slanted side + base cap (32 sectors: 99 vertices, 64 triangles)

Height h = 1, base radius r = 0.5. The surface normal of a cone's side is tilted upward by the slope:

```
side normal at angle θ = normalize( h·sinθ,  r,  h·cosθ ) = normalize( sinθ, 0.5, cosθ )
```

(derivation: the side is the surface F = x² + z² − (r/h)²(½ − y)² = 0; its gradient ∇F is the normal).

* Base ring: `sectors + 1` vertices at y = −0.5 with the side normal of their angle, uv (j/sectors, 0).
* Apex: the apex belongs to every side triangle, but each needs a different normal, so it is stored
  **once per sector** with the normal of the middle of that sector (θ + half a sector). uv (…, 1).
* Side triangle j = `(base_j, base_{j+1}, apex_j)` — bottom-left, bottom-right, top: CCW from outside.
* Base cap: a fan like the cylinder's bottom cap (normal −Y).

Counts: 33 base + 32 apex + 34 cap = 99 vertices; 32 side + 32 cap = 64 triangles.

Used for: lamp shade, horse ears, tail tip, ghost's sheet.

## 6. From primitive to object

A primitive never appears at unit size. Every visible part is a scene node with
`mesh = &assets.Cube()` (etc.), a material, and a local transform built with `SceneNode::AddShape`:

```cpp
// Woody's boot: a cube 0.18 wide, 0.10 tall, 0.30 long, 0.80 below the hip joint, pushed 5 cm forward
hip->AddShape("Foot", &a.Cube(), &boots, { 0, -0.80f, 0.05f }, { 0.18f, 0.10f, 0.30f });
```

How nodes are combined into characters is covered in [06 — Scene graph](06-scene-graph-hierarchy.md) and
[07 — Characters](07-characters-and-props.md).

## 7. Efficiency notes

* 5 VBOs / EBOs for the entire scene; nothing is re-uploaded per frame.
* `GL_UNSIGNED_INT` indices, interleaved attributes (one buffer, good cache locality).
* Sphere/cylinder/cone resolution is chosen once (18×32 / 32 sectors): smooth silhouettes without
  wasting vertices on tiny eyes.
