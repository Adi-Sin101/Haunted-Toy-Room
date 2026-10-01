# 12 — Ray Tracing (GPU, Whitted style)

Files: `src/render/RayTracer.*`, `shaders/raytrace.vert`, `shaders/raytrace.frag`, `shaders/present.frag`,
`src/math/Ray.*` (the CPU twin, used for mouse picking and Buzz's laser), `shaders/lighting.glsl` (the
shared light model).

Press **F4** (or use the Settings panel) to switch the live, fully interactive scene from rasterisation to
ray tracing. Everything keeps working — driving, camera, story, Penny's arrival, edit mode — only the image
is produced differently. **−** / **=** change the ray-tracing resolution (20 %–100 % of the window),
**9** cycles the number of bounces (0–4).

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
hit  ⟺  tNear ≤ tFar  and  tNear > ε ;      the axis that produced tNear is the face hit, normal = −sign(d'_a)·e_a
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
2. **Group** shapes by owner (group 0 = room and scenery, then Woody, Jessie, …, Penny; at most 32 groups).
   For each group compute a **bounding sphere** around its shapes; each unit primitive fits in a sphere of
   radius `√3/2 × its largest axis scale` around its centre.
3. Pack every shape into 8 RGBA32F texels of a **texture buffer** (`GL_TEXTURE_BUFFER`, core since GL 3.1):

   | texel | contents |
   |---|---|
   | 0–2 | rows 0–2 of the inverse model matrix M⁻¹ |
   | 3 | colour.rgb, primitive type (0 plane, 1 cube, 2 sphere, 3 cylinder, 4 cone) |
   | 4 | ka, kd, ks, shininess |
   | 5 | emissive.rgb, opacity |
   | 6 | reflectivity, texture slot (−1 none), uvScale.xy |
   | 7 | unlit flag |

4. Upload group spheres and index ranges as uniform arrays, the lights (the same `uLights[]` uniforms as
   the rasteriser, uploaded by the same function) and a per-light shadow flag.

Rebuilding this every frame keeps the ray tracer live: animation, driving and editing all show up
immediately. Nothing is allocated per frame (the vectors are reused).

## 5. The per-pixel algorithm (`raytrace.frag`)

A full-screen triangle (vertices generated from `gl_VertexID`: (−1,−1), (3,−1), (−1,3); no vertex buffer)
runs the fragment shader once per pixel of a reduced-resolution framebuffer.

```
ray = primary ray (§2);  colour = 0;  throughput = 1
repeat up to (bounces + 1) times:
    hit = nearest intersection over all groups whose bounding sphere the ray hits before the current best t
    if no hit: colour += throughput · background; stop
    P = o + t·d;  N = world normal (flipped to face the ray)
    albedo = colour × texture(uv from the analytic hit point, §6)
    if unlit: shaded = emissive + albedo
    else:
        for each enabled light:
            L, attenuation, spot factor               (lightVector() from lighting.glsl)
            vis = light casts shadows ? (occluded(P + 0.002·N, L, distance − 0.004) ? 0 : 1) : 1
            accumulate diffuse + specular × vis       (addLight() from lighting.glsl)
        shaded = albedo · (ka·Ia + diffuse) + specular + emissive
    if opacity < 0.99:  colour += throughput · opacity · shaded;  throughput ·= (1 − opacity);  continue straight on
    else:               colour += throughput · (1 − reflectivity) · shaded
                        if reflectivity = 0: stop
                        throughput ·= reflectivity;  d = d − 2(d·N)N  (mirror);  o = P + 0.002·N
    stop early when max(throughput) < 0.02
```

* **Group test.** For a group sphere (centre C, radius ρ) and a unit ray: `oc = o − C`, `b = oc·d`,
  `c = oc·oc − ρ²`; skip the group if `b² − c < 0` (miss) or its entry `−b − √(b² − c)` is beyond the
  current nearest hit; always test it if `c < 0` (ray starts inside). A ray that misses Woody's sphere never
  tests his 80 shapes.
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

The sample is `texture(slot, uv · uvScale)`. GLSL 3.30 cannot index an array of samplers with a run-time
value, so the 13 texture slots (`Assets::TextureSlot`: floor, wall, rug, ball, block, poster, stars, moon,
cotton, denim, leather, plaid, cow print) are 13 separate uniforms selected with `if`. The house's exterior
textures (siding, shingles, brick, grass) have no slot, so in ray-traced mode the house shows their tint
colours.

## 7. Output

The traced image (default 50 % resolution) is rendered into a colour texture attached to a framebuffer
object, then drawn over the whole window with bilinear filtering (`present.frag`). A blit cannot be used
because the window's framebuffer is multisampled (4× MSAA).

## 8. Cost

Per pixel: one primary ray plus, per bounce, one shadow ray per shadow-casting light (sun/moon, lamp bulb,
lamp spot), each testing the shapes of every group whose sphere it enters. At 50 % of 1600 × 900 that is
360 000 primary rays per frame. Measured on the development machine (`--benchmark`, story playing):
about 9.6 ms per frame (~100 FPS) in the room. During Penny's arrival the house's ~220 exterior and interior shapes are in
group 0 (always tested), so ray tracing outdoors is about twice as expensive (19.0 ms against 9.3 ms). **−** lowers the
resolution, **9** the bounce count.

## 9. Mouse picking and Buzz's laser use the same maths on the CPU

`RayIntersect::Object(type, inverse(model), ray)` in `src/math/Ray.cpp` implements the identical
intersection routines in C++. Clicking the window casts one ray through the mouse and selects the nearest
owner ([05](05-camera.md)); Buzz's laser casts one ray from his wrist and pushes the first block it hits
([15](15-physics-and-interface.md)).
