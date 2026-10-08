# 03 — Primitives: Vertices, Indices, Triangles

Every object in the scene — room, furniture, five characters, ghost — is assembled from **five
primitive shapes** whose vertices and indices we write ourselves in `src/geometry/Primitives.cpp`.
Each shape is uploaded to the GPU **once** (`Assets::Load`) and shared: every sphere in the scene (heads,
eyes, hands, the ball, the sun…) is drawn from the same sphere VBO with a different model matrix.

The sphere, cylinder and cone exist in **three levels of detail** (full, medium, low) so that small or
distant parts are drawn with far fewer triangles (section 8). The plane and cube are already minimal and
have a single version.

| Mesh | Unit shape (object space) | Vertices | Triangles |
|---|---|---|---|
| Plane | 1 × 1 square in XZ at y = 0, facing +Y | 4 | 2 |
| Cube | [−0.5, 0.5]³ | 24 | 12 |
| Sphere, full | radius 0.5, 24 stacks × 36 sectors | 925 | 1 656 |
| Sphere, medium | 12 stacks × 18 sectors | 247 | 396 |
| Sphere, low | 6 stacks × 10 sectors | 77 | 100 |
| Cylinder, full | radius 0.5, y ∈ [−0.5, 0.5], capped, 32 sectors | 134 | 128 |
| Cylinder, medium | 16 sectors | 70 | 64 |
| Cylinder, low | 8 sectors | 38 | 32 |
| Cone, full | base radius 0.5 at y = −0.5, apex at y = +0.5, capped, 32 sectors | 99 | 64 |
| Cone, medium | 16 sectors | 51 | 32 |
| Cone, low | 8 sectors | 27 | 16 |

The counts follow closed formulas (`n` = sectors, `s` = stacks):

| Shape | Vertices | Triangles |
|---|---|---|
| Plane | 4 | 2 |
| Cube | 6 faces × 4 = 24 | 6 faces × 2 = 12 |
| Sphere | (s + 1)(n + 1) | 2n(s − 1) |
| Cylinder | 2(n + 1) side + 2(n + 2) caps = 4n + 6 | 2n side + 2n caps = 4n |
| Cone | (n + 1) base ring + n apexes + (n + 2) cap = 3n + 3 | n side + n cap = 2n |

**Why unit size?** A model matrix scale of `(w, h, d)` then produces an object of exactly `w × h × d`
units, so the numbers in the character builders read like real dimensions ("a leg 0.15 wide and 0.62
long"). It also lets the ray tracer intersect the exact analytic unit shape in object space
([12 — Ray tracing](12-ray-tracing.md)).

**Rule for every triangle:** its three vertices are listed **counter-clockwise when seen from outside**.
That is OpenGL's front face, so back-face culling removes only hidden faces. How to check it: pick a viewer
outside the face, write down the screen positions (right, up) of the three vertices a → b → c, and compute
the 2D cross product `(b−a) × (c−a)`; positive = counter-clockwise.

Each vertex stores `position`, `normal`, `uv` (see [02](02-opengl-pipeline-fundamentals.md)) — three
floats, three floats and two floats, **32 bytes** per vertex. An index is a 4-byte `GLuint`, three per
triangle (12 bytes per triangle).

### Polygons, triangles and why everything is a triangle

A **polygon** is a flat shape bounded by straight edges. Modelling is easiest with quadrilaterals (a cube
face, a floor, a wall), but a GPU rasterises **only triangles**, for three reasons:

1. Three points are always **coplanar**; four points need not be, so a quad has no single well-defined
   normal or interpolation.
2. A triangle is always **convex**, so "is this pixel inside?" is three half-plane tests.
3. Interpolating a value across a triangle is a *unique* linear function (barycentric coordinates); across
   a general polygon it is ambiguous.

An `n`-sided convex polygon becomes `n − 2` triangles. So a quad → 2 triangles (cube faces, plane, wall),
and a cylinder cap with `n` rim vertices is a **triangle fan** of `n` triangles around its centre vertex
(the centre is an extra vertex, which is why a fan has `n` triangles rather than `n − 2`: it keeps the cap's
uv mapping a perfect disk).

Geometry formulas used by the project for a triangle with corners **a**, **b**, **c**:

```
face normal      n = normalize( (b − a) × (c − a) )      points to the side from which a, b, c look counter-clockwise
area             A = ½ · | (b − a) × (c − a) |
barycentric      p = α·a + β·b + γ·c ,  α + β + γ = 1 ,  α, β, γ ≥ 0 inside the triangle
                 α = area(p, b, c) / A ,  β = area(a, p, c) / A ,  γ = area(a, b, p) / A
screen winding   signed area  S = ½ [ (b.x − a.x)(c.y − a.y) − (c.x − a.x)(b.y − a.y) ]
                 S > 0 → counter-clockwise → front face (kept) ;  S < 0 → back face (culled)
```

The barycentric weights are exactly what the rasteriser uses to interpolate normals, uvs and colours
(see [09 — Shading](09-shading.md)).

**Indexed drawing.** A vertex shared by several triangles is stored once and referenced by index. For
the full sphere, 1 656 triangles use only 925 vertices; without indices it would need
1 656 × 3 = 4 968 vertices (about 5.4× the vertex data and 5.4× the vertex-shader work, minus whatever the
GPU's post-transform cache recovers). Where neighbouring triangles need *different* normals or uvs
(cube edges, cylinder caps, the texture seam) the vertex is deliberately duplicated — a vertex is a
*full bundle of attributes*, not just a position.

**Memory.** Total GPU memory of all eleven meshes ≈ 82 KB (54 KB of vertices + 30 KB of indices):

| Mesh | Vertex bytes (×32) | Index bytes (×12 per triangle) |
|---|---|---|
| Sphere full / medium / low | 29 600 / 7 904 / 2 464 | 19 872 / 4 752 / 1 200 |
| Cylinder full / medium / low | 4 288 / 2 240 / 1 216 | 1 536 / 768 / 384 |
| Cone full / medium / low | 3 168 / 1 632 / 864 | 768 / 384 / 192 |
| Plane / cube | 128 / 768 | 24 / 144 |

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

The full-detail sphere uses 24 stacks × 36 sectors (925 vertices, 1 656 triangles); the medium and low
versions are produced by the **same function** with fewer stacks and sectors (section 8). Smooth shading
interpolates the sphere's exact normals (`n = p / r`), so even the 100-triangle version looks round in the
interior of the shape; only its silhouette is visibly polygonal, and only when it is large on screen —
which is exactly when the full version is chosen.

Edge-length check: with 36 sectors a sphere of radius `r` has a silhouette chord of
`2 r sin(π/36) ≈ 0.174 r`, i.e. the polygonal silhouette deviates from a true circle by at most
`r (1 − cos(π/36)) ≈ 0.0038 r` (0.4 %) — below one pixel for any sphere with a radius under ~260 px.

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

## 7. How many shapes and triangles are in the scene

`bin\Release\HauntedToyRoom.exe --benchmark 5` prints the shape count and the triangles each object
would cost at full detail (measured from the real draw list):

| Object | Shapes | Triangles at full detail |
|---|---|---|
| Room and scenery during the story (walls, desk, bed, bookcase, curtains, window, hall, doors…) | 97 | 15 558 |
| … during Penny's arrival (+ house exterior, garden, ground floor, stairs) | 232 | 57 402 |
| Penny the cat | 34 | 34 656 |
| Woody | 62 | 50 328 |
| Jessie | 63 | 53 628 |
| Bullseye | 44 | 28 264 |
| Buzz | 76 | 63 828 |
| RC Car | 32 | 6 360 |
| Ball | 1 | 1 656 |
| Desk lamp | 5 | 3 632 |
| Doorway crate + 6 wooden blocks | 7 | 12 each |

(In raster mode with lighting on, 5 flattened contact-shadow spheres are added to the scenery.) During the
story everything at full detail is ≈ 258 000 triangles in 421 shapes (it was 278 000 in 492 shapes before
the last round of texture replacements); the frame actually draws about 28 000 – 50 000 because
of level of detail, frustum culling and shadow-pass culling ([17](17-performance.md)). A humanoid is
mostly **spheres**: Woody has 62 shapes, of which 29 are spheres (skull, eyes, pupils, irises, ears, cheeks,
nose, chin, hair, knees, toes, shoulder/elbow/hand balls, thumbs, finger blocks, scarf knot), each worth
1 656 triangles at full detail — 29 × 1 656 = 48 024 of his 50 328 triangles (95 %); his 16 cylinders,
16 cubes and 1 cone add 2 048 + 192 + 64. That is why a small sphere must not use
the full mesh.

*Check, the desk lamp:* Base cylinder 128 + Arm cylinder 128 + Elbow sphere 1 656 + Shade cone 64 +
Bulb sphere 1 656 = **3 632** ✓.

## 8. Level of detail (LOD)

A triangle smaller than a pixel contributes nothing visible but still costs vertex work, so each sphere,
cylinder and cone has three meshes (`Assets::Load`, `Mesh::SetDetailLevels`). Every frame
`Renderer::CollectNode` computes, for each shape:

```
radius      = 0.5 * max length(a0 +/- a1 +/- a2), using four opposite-corner pairs
                (encloses the transformed unit cube, including shear)
distance    = | shape centre − camera position |
screenSize  = radius / distance                        ≈ tangent of the angular radius, proportional to size in pixels

screenSize <  0.012  → low mesh
screenSize <  0.060  → medium mesh
otherwise            → full mesh
```

*Example, Woody's skull* (scale 0.34 × 0.38 × 0.34): `radius = ½·sqrt(0.1156 + 0.1444 + 0.1156) = 0.306`.
It uses the full mesh closer than `0.306 / 0.06 = 5.1` units, the medium mesh up to `0.306 / 0.012 = 25.5`
units and the low mesh beyond. From the default camera (≈ 10 units away) the skull is drawn with 396
triangles instead of 1 656.

The shape's **world matrix is unchanged**, only the mesh pointer differs, so the ray tracer — which does
not use triangles at all — is unaffected: it always intersects the exact analytic shape.

## 9. Efficiency notes

* 11 VBOs / EBOs for the entire scene; nothing is re-uploaded per frame.
* `GL_UNSIGNED_INT` indices, interleaved attributes (one buffer, good cache locality).
* What a texture swap saves, in this file's terms: one shelf's 6 book cubes were 6 × 12 = 72 triangles,
  6 draw calls, 6 matrices and 6 ray-tracer intersection tests per ray; the textured box is 12 triangles and
  1 of each (all 4 shelves: 288 → 48 triangles, 24 → 4 shapes). The fence's ~50 picket cubes + 4 rails
  (≈ 650 triangles, ~55 shapes) are now 2 boxes (24 triangles). For spheres the saving is bigger: each
  removed shirt button or eye glint was up to 1 656 triangles.
* Surface detail that used to be modelled with extra shapes (stitching, braid beads, tyre treads,
  quilt seams, studs, mane strands — over 200 shapes) now comes from textures
  ([10 — Textures](10-textures.md)): a texture costs the same however much detail it shows, a shape
  costs triangles, draw calls and a matrix every frame.
