# 12 — Ray Tracing (GPU, Whitted style)

Files: `src/render/RayTracer.*`, `shaders/raytrace.vert`, `shaders/raytrace.frag`, `shaders/present.frag`,
`src/math/Ray.*` (the CPU twin, used for mouse picking and Buzz's laser), `shaders/lighting.glsl` (the
shared light model).

Press **F4** (or use the Settings panel) to switch the live, fully interactive scene from rasterisation to
ray tracing. Everything keeps working — driving, camera, story, Penny's arrival, edit mode — only the image
is produced differently. **−** / **=** change the ray-tracing resolution (20 %–100 % of the window),
**Ctrl+9** cycles the number of bounces (0–4).

**Missed rays see the sky.** A ray that hits no instance returns `skyRadiance(direction)` from
`shaders/sky.glsl`, the same function the rasteriser's sky pass uses, so the ray-traced and rasterised
skies are identical and reflections and glass show the real sky. Outdoors light 0 comes from the visible
sun or moon, so its shadow rays give correct garden shadows. See
[21 — Outdoor sky and solid characters](21-outdoor-sky-and-solid-characters.md).

![Ray-traced room](images/raytraced.png)

*F4: the same live scene ray traced — reflections of the ball and toys on the polished floor, walls block the sunlight.*

## 1. Rasterisation vs ray tracing

| Rasterisation | Ray tracing |
|---|---|
| for each triangle → which pixels does it cover? | for each pixel → which surface does its ray hit first? |
| lighting is local: no knowledge of other objects | can ask "is anything between this point and the light?" |
| shadows need a shadow map, mirrors and refraction need extra passes | shadows, reflections and transparency fall out naturally |
| cost ∝ triangles + pixels | cost ∝ pixels × objects tested per ray × rays per pixel |

## 2. Rays

A ray is `p(t) = o + t·d` with origin o, direction d and parameter t > 0 (distance along the ray when d is
a unit vector).

**Primary rays** (one per pixel). For a pixel whose centre has NDC coordinates (x, y) ∈ [−1, 1]², with the
camera's unit axes forward f, right r, up u:

```
d = normalize( f + x · tan(fov/2) · aspect · r + y · tan(fov/2) · u )         o = camera position
```

This is the inverse of the perspective projection of [09 §1](09-shading.md): a point on the image plane at
distance 1 in front of the camera spans `±tan(fov/2)·aspect` horizontally and `±tan(fov/2)` vertically.

## 3. Rays against exact primitives, not triangles

Because the whole scene is built from five unit primitives, the tracer does not test triangles at all.
It transforms the ray into each shape's **object space**, where the shape is the unit primitive:

```
o' = M⁻¹ · (o, 1)        d' = M⁻¹ · (d, 0)          M = the shape's world matrix
```

d' is **not re-normalised**. Then `o' + t·d' = M⁻¹(o + t·d)` for every t, so the t found in object space is
the same t as in world space, and hits on different shapes can be compared directly. Scale, rotation,
shear and reflection of the shape are all handled by M⁻¹ — the intersection code only ever sees the unit
shape. (Level of detail does not affect the ray tracer: it never uses the meshes.)

### 3.1 The intersection equations (`raytrace.frag`, mirrored in `src/math/Ray.cpp`)

**Plane** (y = 0, |x|, |z| ≤ ½, facing +Y). `o'_y + t·d'_y = 0 → t = −o'_y / d'_y`; accept if
|x|, |z| ≤ ½ at the hit. It is **one-sided**: only rays coming from above (`d'_y < 0`, `o'_y > 0`) can hit,
like back-face culling. So walls are seen from inside only, and sunlight is blocked by the outer walls.

**Cube** ([−½, ½]³) — the slab method. The cube is the intersection of three slabs `−½ ≤ x ≤ ½` etc. For
each axis the ray is inside the slab for t between
`t₁ = (−½ − o'_a)/d'_a` and `t₂ = (½ − o'_a)/d'_a`. It is inside the cube where it is inside all three:

```
tNear = max over axes of min(t₁, t₂)        tFar = min over axes of max(t₁, t₂)
hit  ⟺  tNear ≤ tFar  and  tFar > ε
use tNear when tNear > ε; otherwise use tFar (the exit face for an inside origin)
entry normal = −sign(d'_a)·e_a; exit normal = sign(d'_a)·e_a, using the chosen face axis
```

**Sphere** (|p| = ½). Substitute the ray: `|o' + t·d'|² = ¼` →

```
a t² + b t + c = 0    with   a = d'·d',  b = 2 o'·d',  c = o'·o' − ¼
discriminant Δ = b² − 4ac:  Δ < 0 → miss;  else t = (−b − √Δ) / 2a  (the nearer root; if ≤ ε, the farther one)
object normal = (o' + t·d') / ½  (a sphere's normal points away from its centre)
```

**Cylinder** (x² + z² = ¼, |y| ≤ ½, with caps). The side ignores y:

```
a = d'x² + d'z²,  b = 2(o'x d'x + o'z d'z),  c = o'x² + o'z² − ¼     roots accepted only if |o'y + t d'y| ≤ ½
normal = (x, 0, z) / ½
caps: t = (±½ − o'y) / d'y, accepted if x² + z² ≤ ¼ at the hit, normal (0, ±1, 0)
```

The nearest accepted t of the side and the two caps wins.

**Cone** (apex at y = ½, base radius ½ at y = −½). The radius shrinks linearly with height:
`r(y) = k(½ − y)` with `k = ½`, so the side is `F(x, y, z) = x² + z² − k²(½ − y)² = 0`. Substituting the ray
and writing `h = ½ − o'y` (height of the origin below the apex):

```
a = d'x² + d'z² − k² d'y²
b = 2( o'x d'x + o'z d'z + k² h d'y )
c = o'x² + o'z² − k² h²                    roots accepted only if −½ ≤ y ≤ ½ (one nappe of the double cone)
normal = ∇F = ( 2x, 2k²(½ − y), 2z )        then normalised;  base cap as for the cylinder (y = −½, normal −Y)
```

When `|a|` is nearly zero the side equation becomes linear: use `t = -c/b` when `|b|` is nonzero. This handles rays parallel to a cone generator without division by a vanishing quadratic coefficient.

### 3.2 The world normal

The object-space normal n' is brought to world space with the same normal matrix as in rasterisation,
`N = normalize( (M⁻¹)ᵀ n' )` ([08 §8](08-illumination.md)). The tracer already stores the rows of M⁻¹, so
`(M⁻¹)ᵀ n' = n'x·row₀ + n'y·row₁ + n'z·row₂`. If N points along the ray (`N·d > 0`, an inside or back hit)
it is flipped.

### 3.3 Worked example: a primary ray hitting the beach ball

The ball is a unit sphere scaled by 0.7 (radius 0.35) at c = (−3.5, 0.35, 3.5): `M = T(c)·S(0.7)`, so
`M⁻¹ p = (p − c) / 0.7`. The ray starts at the default camera o = (0, 5, 8.2) with
d = (−0.4573, −0.6255, −0.6322).

| Step | Value |
|---|---|
| o' = (o − c)/0.7 | (5.000, 6.643, 6.714) |
| d' = d/0.7 | (−0.6533, −0.8935, −0.9031) — length 1/0.7, not normalised |
| a = d'·d' | 2.0408 (= 1/0.49) |
| b = 2 o'·d' | −30.532 |
| c = o'·o' − ¼ | 113.959 |
| Δ = b² − 4ac | 1.909 → hit |
| t = (−b − √Δ)/2a | (30.532 − 1.382) / 4.082 = **7.142** (far root 7.819) |
| world hit o + t·d | (−3.266, 0.533, 3.685) — its distance to c is exactly 0.350 ✓, so t is a world distance |
| object normal (o' + t d')/½ | (0.668, 0.523, 0.529); uniform scale → same world direction |

The ray enters the ball 7.142 units from the camera and leaves it 0.677 later; the entry point faces up and
towards the camera, as expected.

## 4. Getting the scene to the GPU (`RayTracer::Render`, CPU side, every frame)

1. Take the renderer's flattened draw list (only visible shapes: the house exterior and the ground floor
   are hidden once Penny is upstairs, so they cost nothing then).
2. Compute the **world-space axis-aligned box** of every shape. A unit primitive lies in [−½, ½]³, so under
   M = [a₀ a₁ a₂ | c] its box is centred on c with half-size `½(|a₀| + |a₁| + |a₂|)` (component-wise absolute
   values of the axis columns).
3. Build a **bounding volume hierarchy** (§4.1) over those boxes.
4. Pack every shape, in the BVH's leaf order, into 8 RGBA32F texels of a **texture buffer**
   (`GL_TEXTURE_BUFFER`, core since GL 3.1), followed by the BVH nodes (2 texels each):

   | texel | shape contents |
   |---|---|
   | 0–2 | rows 0–2 of the inverse model matrix M⁻¹ |
   | 3 | colour.rgb, primitive type (0 plane, 1 cube, 2 sphere, 3 cylinder, 4 cone) |
   | 4 | ka, kd, ks, shininess |
   | 5 | emissive.rgb, opacity |
   | 6 | reflectivity, texture slot (−1 none), uvScale.xy |
   | 7 | unlit flag, cut-out flag |

   | texel | node contents |
   |---|---|
   | 0 | box min.xyz, `first` |
   | 1 | box max.xyz, `count` (> 0: a leaf holding shapes first … first + count − 1; 0: an inner node whose children are nodes `first` and `first + 1`) |

   **M⁻¹ without inverting a 4 × 4.** M = [A | t] is affine, so `M⁻¹ = [A⁻¹ | −A⁻¹ t]`, and the renderer
   already has the normal matrix `N = (A⁻¹)ᵀ` for every shape. Row r of A⁻¹ is column r of N, so each packed
   row is `(N[r], −N[r]·t)` — no matrix inversion per shape per frame.
5. Upload the lights (the same `uLights[]` uniforms as the rasteriser, uploaded by the same function) and a
   per-light shadow flag.

Rebuilding this every frame keeps the ray tracer live: animation, driving and editing all show up
immediately. Nothing is allocated per frame (the vectors are reused).

### 4.1 The bounding volume hierarchy (BVH)

Without an acceleration structure every ray would have to test every shape: with ~420 shapes, 360 000
primary rays and up to three shadow rays and a reflection per pixel, that is hundreds of millions of
intersection tests per frame. A BVH is a binary tree of boxes: every node's box encloses all the shapes
below it, so if a ray misses a node's box it can skip that whole subtree.

**Build** (`RayTracer::Build`, top-down, recursive):

```
node box   = union of the boxes of its shapes
leaf       if it holds ≤ 2 shapes (or all centres coincide)
otherwise  axis  = longest axis of the box CENTRES' extent
           split = median: std::nth_element puts the half with the smaller centre coordinate first
           children = Build(first half), Build(second half), stored next to each other
```

The median split always halves the shape count, so the tree has depth ⌈log₂(n/2)⌉ ≈ 8 for 420 shapes and
about n nodes (2 × leaves − 1). Building it costs O(n log n) — a fraction of a millisecond — which is why it can simply
be rebuilt every frame while the toys move.

**Traversal** (`traceClosest` in `raytrace.frag`), with an explicit stack because GLSL has no recursion:

```
push root
while the stack is not empty:
    pop node
    if the ray misses node.box, or enters it beyond the nearest hit so far: continue
    if leaf: intersect its ≤ 2 shapes exactly, keep the nearest hit (this shrinks tBest)
    else:    test both children's boxes; push the farther one first, so the nearer one is popped first
```

Ray–box test (slab method, with precomputed `1/d`):
`t₁ = (lo − o)·(1/d)`, `t₂ = (hi − o)·(1/d)`, `tNear = max(min(t₁, t₂), 0)`, `tFar = min(max(t₁, t₂))`;
the ray enters the box iff `tNear ≤ tFar` and `tNear < tBest`.

Visiting the nearer child first finds a close hit early; every later box that starts beyond it is then
rejected with one box test. **Shadow rays** (`occluded`) use the same traversal as an *any-hit* query: they
stop at the first opaque blocker closer than the light. A ray now does about 2·log₂ n box tests plus a few
exact shape tests, instead of testing whole objects (the previous version tested all 97 room shapes for
every ray, and all 60–90 shapes of any toy whose bounding sphere the ray touched).

### 4.2 Worked example: a four-shape BVH

Four unit cubes A, B, C, D in a row along x: A = [0, 1] × [0, 1] × [0, 1], B = [2, 3] × …, C = [6, 7] × …,
D = [8, 9] × … (all with y, z ∈ [0, 1]). Box centres x = 0.5, 2.5, 6.5, 8.5.

**Build.** Root box = union = [0, 9] × [0, 1] × [0, 1]. The centres spread 8 units in x and 0 in y, z →
split on x. The median puts {A, B} left and {C, D} right. Left box [0, 3] × [0, 1]², right box [6, 9] × [0, 1]².
Each child holds 2 shapes = LeafSize → leaves. Nodes: 0 = root (first = 1, count = 0), 1 = left leaf
(first = 0, count = 2), 2 = right leaf (first = 2, count = 2); the shapes are packed in the order A, B, C, D.

**A ray that hits.** o = (−1, 0.5, 0.5), d = (1, 0, 0). `1/d` = (1, 10⁸, 10⁸) (`safeInverse` replaces a 0
component by 10⁻⁸ so no division by zero produces NaN).

| Step | Slabs | tNear / tFar | Result |
|---|---|---|---|
| root [0,9]×[0,1]² | x: (0+1)·1 = 1, (9+1)·1 = 10; y, z: (0−0.5)·10⁸ = −5·10⁷, (1−0.5)·10⁸ = 5·10⁷ | max(1, −5·10⁷, −5·10⁷, 0) = 1 / min(10, 5·10⁷, 5·10⁷) = 10 | enter at t = 1 |
| left child [0,3] | x: 1 … 4 | 1 / 4 | entry 1 |
| right child [6,9] | x: 7 … 10 | 7 / 10 | entry 7 |
| push order | right (farther) first, then left | | pop left first |
| left leaf: A, B | exact cube tests | A at t = 1, B at t = 3 | best t = 1 (A) |
| pop right | its box starts at 7 > best 1 | | rejected with one box test |

Total: 3 box tests + 2 exact shape tests (instead of 4 shape tests); with 420 shapes a typical ray needs
~16 box tests + a handful of shape tests instead of hundreds of shape tests.

**A ray that misses.** o = (−1, 2, 0.5), d = (1, 0, 0): the root's y-slab gives t ∈ [(0 − 2)·10⁸, (1 − 2)·10⁸]
= [−2·10⁸, −10⁸], so tFar < tNear → the ray misses the whole scene with **one** box test.

## 5. The per-pixel algorithm (`raytrace.frag`)

A full-screen triangle (vertices generated from `gl_VertexID`: (−1,−1), (3,−1), (−1,3); no vertex buffer)
runs the fragment shader once per pixel of a reduced-resolution framebuffer.

```
ray = primary ray (§2);  colour = 0;  throughput = 1
repeat up to (bounces + 1) times:
    hit = nearest intersection, found by descending the BVH (§4.1); cut-out holes do not count (§6)
    if no hit: colour += throughput · background; stop
    P = o + t·d;  N = world normal (flipped to face the ray)
    albedo = colour × texture(uv from the analytic hit point, §6)
    if unlit: shaded = emissive + albedo
    else:
        for each enabled light:
            L, attenuation, spot factor               (lightVector() from lighting.glsl)
            strength = attenuation · intensity · N·L
            if strength < 0.004: skip the light        (it cannot change the pixel visibly)
            vis = light casts shadows and strength > 0.02 ? (occluded(P + 0.002·N, L, distance − 0.004) ? 0 : 1) : 1
            accumulate diffuse + specular × vis       (addLight() from lighting.glsl)
        shaded = albedo · (ka·Ia + diffuse) + specular + emissive
    if opacity < 0.99:  colour += throughput · opacity · shaded;  throughput ·= (1 − opacity);  continue straight on
    else:               colour += throughput · (1 − reflectivity) · shaded
                        if reflectivity = 0: stop
                        throughput ·= reflectivity;  d = d − 2(d·N)N  (mirror);  o = P + 0.002·N
    stop early when max(throughput) < 0.02
```

* **Only useful shadow rays.** A shadow ray is a full BVH traversal, so it is only sent where the light can
  visibly change the pixel: lights whose `attenuation · intensity · N·L` is below 0.004 are skipped, and
  below 0.02 they are added unshadowed (an error of at most 0.02 in a 0–1 colour channel). Example, the
  lamp bulb (intensity 0.8, kl 0.14, kq 0.07) on a surface with N·L = 0.3:

  | distance | attenuation | strength | treatment |
  |---|---|---|---|
  | 5 | 0.290 | 0.070 | lit with a shadow ray |
  | 12 | 0.078 | 0.019 | lit, no shadow ray |
  | 25 | 0.021 | 0.005 | lit, no shadow ray |
  | 30 | 0.015 | 0.0035 | skipped |
* **Shadow rays** (`occluded`) are *any-hit* tests with a maximum distance: the first opaque, lit
  intersection closer than the light ends the search. Light sources (unlit shapes such as the bulb, sky,
  sun) and transparent glass do not cast shadows.
* **Self-shadowing.** The offset `0.002·N` ("shadow acne" epsilon) keeps a surface from shadowing itself
  because of floating-point error in the hit position.
* **Reflection.** `d − 2(d·N)N` mirrors the ray direction about the surface; e.g. the floor (N = (0,1,0))
  turns d = (0.303, −0.505, −0.808) into (0.303, 0.505, −0.808). The colour seen is the weighted sum
  `(1 − reflectivity)·local + reflectivity·reflected`.
* **Transparency** composites front to back: a surface of opacity α contributes `throughput·α·colour` and
  lets `1 − α` of the light from behind through — the same "over" operator as alpha blending, evaluated
  along the ray.
* Reflective materials: floor 0.18, car paint 0.15, car glass 0.3, ball 0.12, house window glass 0.2,
  Buzz's helmet (glass: opacity 0.22, reflectivity 0.25).
* Sunlight enters **only through the window opening**: the walls block it, which rasterisation cannot do.

## 6. Textures from the hit point (`primitiveUV`)

The uv is computed analytically from the object-space hit point so that it matches the mesh uvs of
[03](03-primitives.md) exactly:

| Shape | u | v |
|---|---|---|
| Plane | x + ½ | ½ − z |
| Sphere | atan2(x, z) / 2π (wrapped to [0, 1)) | ½ + asin(2y) / π |
| Cube | per face, the in-face coordinates + ½ (same orientation as the mesh faces) | |
| Cylinder / cone side | atan2(x, z) / 2π | y + ½ |
| Caps | ½ + x | ½ ∓ z |

For the sphere: the mesh puts ring i at latitude `φ = 90° − i·180°/stacks` with `v = 1 − i/stacks = ½ + φ/π`,
and `y = ½ sin φ` → `φ = asin(2y)`; longitude `θ = atan2(x, z)` because `x = r cos φ sin θ`,
`z = r cos φ cos θ`.

The sample is `texture(uSurfaceMaps, vec3(uv * uvScale, slot))`. A single `sampler2DArray` contains 22 mapped layers plus a white fallback: floor, wall, rug, ball, block, poster, stars, moon, cotton, denim, leather, plaid, cow print, book spines, pickets, siding, shingles, brick, grass, window panes, flower bed and the provided hallway clock BMP. Source maps are bilinearly resampled to 512 × 512 per layer and mipmaps are generated. Array-layer selection is legal in GLSL 3.30 and avoids a branch ladder and the previous 15 separate sampler bindings. Exterior maps therefore also appear in ray-traced views. The instance buffer uses texture unit 0, the surface array unit 1 and presentation unit 2.

**Cut-outs.** The picket fence and the porch railings are single boxes whose texture has alpha = 0
between the pickets ([10](10-textures.md)). For a shape with the cut-out flag, every candidate hit — in
`traceClosest` and in the shadow rays — samples the texture's alpha at the hit's uv and is ignored when
`alpha < 0.5`, so rays pass between the pickets and the pickets cast picket-shaped shadows. The alpha is
read from mip level 0 (`textureLod(…, 0)`): neighbouring pixels of a ray tracer can hit unrelated surfaces,
so the screen-space derivatives that normally choose the mip level are meaningless at a cut-out's edges.

## 7. Output

The traced image (default 50 % resolution) is rendered into a colour texture attached to a framebuffer
object, then drawn over the whole window with bilinear filtering (`present.frag`). A blit cannot be used
because the window's framebuffer is multisampled (4× MSAA).

## 8. Cost

Per pixel: one primary ray plus, per bounce, at most one shadow ray per shadow-casting light (sun/moon,
lamp bulb, lamp spot) where that light matters. At 50 % of 1600 × 900 that is 360 000 primary rays per
frame. Measured on the development machine (`--benchmark`, lighting + shading + textures on, story playing):

| | Per-object groups (before) | BVH + shadow-ray culling |
|---|---|---|
| Story, 1600 × 900 | 30.3 ms (33 FPS) | 9.5 ms (105 FPS) |
| Story, 1920 × 1080 | 46.6 ms (21 FPS) | 14.8 ms (68 FPS) |
| Penny's arrival (house visible) | 41.1 ms (24 FPS) | 6.3 ms (159 FPS) |

The arrival gains the most: the house's shapes (about 220 before this round, 135 now) used to be in the
always-tested scenery group, so every ray tested all of them; in the BVH a ray only reaches the few that lie along it. **−** lowers the
resolution, **Ctrl+9** the bounce count. See [17 — Performance](17-performance.md).

## 9. Mouse picking and Buzz's laser use the same maths on the CPU

`RayIntersect::Object(type, inverse(model), ray)` in `src/math/Ray.cpp` implements the identical
intersection routines in C++. Clicking the window casts one ray through the mouse and selects the nearest
owner ([05](05-camera.md)); Buzz's laser casts one ray from his wrist and pushes the first block it hits
([15](15-physics-and-interface.md)).
