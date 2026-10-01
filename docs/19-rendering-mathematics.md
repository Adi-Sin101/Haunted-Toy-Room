# 19 — Rendering Mathematics: One Frame, Formula by Formula

This chapter is a reference that walks through **one frame** in the order the program executes it and
gives every formula used, with links to the chapters that derive them and work through numbers. Notation:
points are column vectors `p = (x, y, z, 1)`, directions `(x, y, z, 0)`; `M·p` applies M to p; `·` is a dot
product between vectors; `×` a cross product; `mix(a, b, t) = a + (b − a)t`;
`smoothstep(e0, e1, x) = t²(3 − 2t)` with `t = clamp((x − e0)/(e1 − e0), 0, 1)`.

## 1. Time

```
dt = min(now − previous, 0.1 s)                         frame time, clamped after stalls
every speed is "per second" and multiplied by dt        frame-rate independent motion
exponential easing:  x += (target − x) · (1 − e^(−k·dt))     identical result for any frame rate
```

## 2. Motion ([07 §5](07-characters-and-props.md), [11](11-environment-animation.md))

```
forward = (sin h, 0, cos h)                     heading h in degrees, 0 = +Z
v += clamp(v* − v, −a·dt, a·dt)                 constant acceleration
h += turn · turnRate · dt ;  p += forward · v · dt
walkPhase += v · dt · 4                         gait driven by distance, so feet do not slide
limb angle = sin(walkPhase) · amplitude · moveBlend
rolling ball:  axis = normalize(up × Δp),  angle = |Δp| / r,  B ← R(axis, angle) · B      (Rodrigues)
wheel:         angle += v · dt / r
```

## 3. Physics ([15](15-physics-and-interface.md), [17](17-performance.md))

```
world box of a unit cube under M:  half = |M₀|·½ + |M₁|·½ + |M₂|·½   (component-wise |column|)
swept box test (slab method on the box expanded by the mover's half-size): first contact time t ∈ [0, 1]
slide:  Δ ← Δ(1 − t);  Δ ← Δ − n·min(0, Δ·n)
blocks: fixed 1/120 s steps, at most 6 per frame;  v.y −= 9.81·step;  p += v·step
contact: overlap on each axis, push out along the smallest; impulse removes the approaching velocity
```

## 4. The scene graph ([04](04-transformations.md), [06](06-scene-graph-hierarchy.md))

```
local  M_l = T(position) · Ry(yaw) · Rx(pitch) · Rz(roll) · B · S(scale)      (multiplied out in closed form)
world  M_w(node) = M_w(parent) · M_l(node)                                     one depth-first pass (MatrixStack)
normal N = transpose(inverse(upper-left 3×3 of M_w))                           ([08 §8])
```

Hidden subtrees are skipped. Lights then copy their positions (`M_w[3]`) and directions (a column of `M_w`)
from scene nodes.

## 5. Collecting the draw list ([03 §8](03-primitives.md))

For every visible shape with world matrix M (axis columns a₀, a₁, a₂, translation c):

```
bounding radius  r = ½ · sqrt(|a₀|² + |a₁|² + |a₂|²)          encloses the scaled unit cube, hence every primitive
view distance    d = |c − eye|
level of detail  s = r / d:   s < 0.012 → low,  s < 0.06 → medium,  else full
```

## 6. Camera ([05](05-camera.md), [09 §1](09-shading.md))

```
forward f = (cos p · sin y, sin p, −cos p · cos y)     yaw y, pitch p
r = normalize(f × (0,1,0)),  u = r × f
V = | r.x  r.y  r.z  −r·eye |        P = | 1/(A·t)  0    0              0          |    t = tan(fov/2), A = aspect
    | u.x  u.y  u.z  −u·eye |            | 0        1/t  0              0          |
    |−f.x −f.y −f.z   f·eye |            | 0        0    −(F+N)/(F−N)  −2FN/(F−N) |    N = 0.05, F = 200
    | 0    0    0     1     |            | 0        0    −1             0          |
```

## 7. Frustum culling ([17](17-performance.md))

A point is inside the view volume when its clip coordinates satisfy `−w ≤ x, y, z ≤ w`. With
`clip = (P·V)·p` and `rowᵢ` the i-th row of P·V, `x_c = row₀·p` and `w = row₃·p`, so
`x_c ≥ −w ⇔ (row₃ + row₀)·p ≥ 0`. The six planes are therefore

```
row₃ + row₀ (left),  row₃ − row₀ (right),  row₃ + row₁ (bottom),  row₃ − row₁ (top),  row₃ + row₂ (near),  row₃ − row₂ (far)
```

Each plane `(n, w)` is divided by `|n|`, so `n·c + w` is the signed distance of a point c from it. A shape
is culled when, for some plane, `n·c + w < −r`: its whole bounding sphere is outside.

## 8. Lamp shadow map ([08 §6.2](08-illumination.md))

```
LightVP = Perspective(2 · outerCone, 1, 0.08, 36) · LookAt(lampPos, lampPos + axis, up)
pass 1: depth of every opaque shape (inside the lamp frustum, r ≥ 0.035) → 2048² depth texture
pass 2: coord = (LightVP · P).xyz / w · ½ + ½;   bias = max(0.0009(1 − N·L), 0.00012)
        vis = (1/9) Σ over 3×3 texels [coord.z − bias ≤ depth(coord.xy + offset)]
```

## 9. Rasterisation ([09](09-shading.md))

```
clip = P · V · M · p ;   ndc = clip.xyz / clip.w ;   window = ((ndc.xy + 1)/2 · size, (ndc.z + 1)/2)
back-face test: signed screen area S = ½[(b−a) × (c−a)]_z < 0 → culled
barycentric weights α, β, γ from sub-triangle areas
perspective-correct attribute: (Σ αᵢ aᵢ / wᵢ) / (Σ αᵢ / wᵢ)
depth test: keep the fragment if its depth < stored depth
```

## 10. Texturing ([10](10-textures.md))

```
uv' = uv · uvScale  (GL_REPEAT wraps to the fractional part)
bilinear:  Σ over the 4 nearest texels of (1−|dx|)(1−|dy|) · texel
mip level: λ = log₂ max(|∂(uW,vH)/∂x|, |∂(uW,vH)/∂y|);   trilinear = mix of levels ⌊λ⌋, ⌈λ⌉
albedo = material colour × texture colour          (greyscale textures are tinted by the colour)
```

## 11. Illumination at a point ([08](08-illumination.md))

```
L = normalize(lightPos − P) or −direction ;  V = normalize(eye − P) ;  H = normalize(L + V) ;  R = 2(N·L)N − L
att  = 1 / (kc + kl·d + kq·d²) ;  spot = smoothstep(cos outer, cos inner, −L·axis)
skip the light (and its shadow lookup) if att·spot·intensity < 10⁻⁴ or N·L ≤ 0
I = C·(ka·Ia + Σ att·spot·vis·Il·kd·max(N·L, 0)) + Σ att·spot·vis·Il·ks·spec + E
spec = max(R·V, 0)^n (Phong)  or  max(N·H, 0)^(4n) (Blinn-Phong)
```

Evaluated per fragment (Flat: N from `dFdx × dFdy` of the position; Phong / Blinn: interpolated N
re-normalised) or per vertex (Gouraud). The selection rim adds `h·(0.12 + 0.6(1 − N·V)²)·(1, 0.8, 0.2)`.

## 12. Blending

```
opaque pass:      front to back in 0.5-unit depth slices, then by material and mesh   (early-z rejects hidden fragments)
cut-outs:         discard the fragment if texture alpha < 0.5
transparent pass: sorted far → near, depth writes off,  colour = α·src + (1 − α)·dst
```

## 13. Ray tracing instead of 9–12 ([12](12-ray-tracing.md))

```
primary ray  d = normalize(f + x·t·A·r + y·t·u)          (x, y) = pixel centre in NDC
object space o' = M⁻¹(o, 1),  d' = M⁻¹(d, 0)              t is the same in both spaces
sphere       (d'·d') t² + 2(o'·d') t + (o'·o' − ¼) = 0
cylinder     (d'x² + d'z²) t² + 2(o'x d'x + o'z d'z) t + (o'x² + o'z² − ¼) = 0,  |y| ≤ ½,  + caps
cone         x² + z² = ¼(½ − y)²  →  a = d'x² + d'z² − ¼ d'y², b = 2(o'x d'x + o'z d'z + ¼ h d'y), c = o'x² + o'z² − ¼ h²,  h = ½ − o'y
cube         slab method: tNear = max(min(t₁, t₂)), tFar = min(max(t₁, t₂)), hit if tNear ≤ tFar
plane        t = −o'y / d'y  (from above only)
normal       N = normalize((M⁻¹)ᵀ n')
BVH          node box ⊇ its shapes; median split on the longest centre axis; traversal with a stack,
             nearer child first; ray–box: tNear = max(min((lo−o)/d, (hi−o)/d), 0) ≤ tFar = min(max(…))
M⁻¹          rows (N[r], −N[r]·t) from the normal matrix N = (A⁻¹)ᵀ — no 4×4 inverse
shadow ray   only if att·intensity·N·L ≥ 0.02; from P + 0.002 N towards the light; any hit before it → vis = 0
reflection   d ← d − 2(d·N)N ;  colour += throughput·(1 − ρ)·local ;  throughput ·= ρ
transparency colour += throughput·α·local ;  throughput ·= (1 − α)
```

## 14. Worked examples elsewhere

| Example | Chapter |
|---|---|
| Sphere vertex and index table (4 stacks × 4 sectors) | [03 §3](03-primitives.md) |
| LOD choice for Woody's skull (r = 0.306: full < 5.1 units, medium < 25.5) | [03 §8](03-primitives.md) |
| A boot vertex from object space to pixel (815.1, 517.8), depth 0.9949 | [09 §1](09-shading.md) |
| Lamp-spot lighting of a floor point: (0.968, 0.581, 0.286); the mirror point's highlight | [08 §9](08-illumination.md) |
| A primary ray hitting the beach ball at t = 7.142 | [12 §3.3](12-ray-tracing.md) |
| The roof pitch (28°) and the one-cube gable matrix | [04 §4.2](04-transformations.md) |
| Penny's jump arc and the arrival's sunset clock | [18 §3](18-house-and-penny.md) |
| Building and traversing a four-shape BVH (3 box tests + 2 shape tests; a miss in 1 test) | [12 §4.2](12-ray-tracing.md) |
| Shadow-ray culling for the lamp bulb at 5 / 12 / 25 / 30 units | [12 §5](12-ray-tracing.md) |
| A penumbra pixel of the lamp's shadow map (vis = 6/9) and a light skipped before the lookup | [08 §6.2](08-illumination.md) |
| Fence cut-out coordinates (uvScale 3.741; picket centre at u' = 0.0625) | [10](10-textures.md) |
| Triangles saved by the texture swaps (books 288 → 48) | [03 §9](03-primitives.md) |
