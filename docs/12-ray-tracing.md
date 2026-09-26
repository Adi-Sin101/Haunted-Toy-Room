# 12 — Ray Tracing (GPU, Whitted style)

Files: `src/render/RayTracer.*`, `shaders/raytrace.vert`, `shaders/raytrace.frag`, `shaders/present.frag`,
`src/math/Ray.*` (CPU twin used for mouse picking), `shaders/lighting.glsl` (shared light model).

Press **F4** to switch the live, fully interactive scene from rasterisation to ray tracing. Everything
keeps working — driving, camera, day/night, edit mode — only the image is produced differently.
**-** / **=** change the ray tracing resolution (20 %–100 % of the window), **9** cycles the number of
bounces (0–4).

![Ray-traced room](images/raytraced.png)

*F4: the same live scene ray traced — reflections of the ball and toys on the polished floor, walls block the sunlight.*

## 1. Rasterisation vs ray tracing

| Rasterisation | Ray tracing |
|---|---|
| for each triangle → which pixels does it cover? | for each pixel → which surface does its ray hit first? |
| lighting is local: no knowledge of other objects | can ask "is anything between this point and the light?" |
| shadows, mirrors, refraction need extra tricks | shadows, reflections, transparency fall out naturally |
| very fast | expensive: many ray–object tests per pixel |

## 2. Rays against exact primitives, not triangles

Because the whole scene is built from five unit primitives, the tracer does not test thousands of
triangles. It tests each part **analytically** against its exact shape:

```
world ray      p(t) = o + t·d
object ray     o' = M⁻¹·(o, 1),   d' = M⁻¹·(d, 0)      (M = the part's world matrix)
```

d' is **not re-normalised**, therefore the `t` found in object space is the same `t` in world space and
hits from different parts can be compared directly. Scale, rotation, shear and reflection of the part are
all handled by M⁻¹ — the intersection code only ever sees the unit shape.

### The five intersection routines (`raytrace.frag`, mirrored in `Ray.cpp`)

| Shape | Equation | Method |
|---|---|---|
| Plane | y = 0, |x|,|z| ≤ 0.5 | `t = −o.y / d.y`; one-sided (only rays coming from above) — like back-face culling, so walls are seen from inside only and the sun's shadow rays are blocked by walls |
| Cube | [−0.5, 0.5]³ | slab method: intersect the ray with the 3 pairs of parallel planes, `tNear = max(t_min)`, `tFar = min(t_max)`; hit if tNear ≤ tFar; the axis of `tNear` gives the face normal |
| Sphere | \|p\|² = 0.25 | quadratic `(d·d)t² + 2(o·d)t + (o·o − 0.25) = 0`; smallest positive root; normal = p / 0.5 |
| Cylinder | x² + z² = 0.25, \|y\| ≤ 0.5 | quadratic in x, z; reject roots outside the height; plus two cap disks |
| Cone | x² + z² = (½(½ − y))², y ∈ [−½, ½] | quadratic; normal = gradient `(2x, ½(½ − y), 2z)`; plus base disk |

**World normal** from the object-space normal: `N = normalize(transpose(M⁻¹)₃ₓ₃ · n')` — the same normal
matrix as in rasterisation, built from the rows of M⁻¹ already in the buffer.

## 3. Getting the scene to the GPU (`RayTracer::Render`, CPU side, every frame)

1. Take the renderer's flattened draw list (same one the rasteriser draws).
2. **Group** items by owner (group 0 = room, then Woody, Jessie, …). For each group compute a
   **bounding sphere** from its parts (each unit primitive fits in a sphere of radius √3/2 × its largest
   scale).
3. Pack every item into 8 RGBA32F texels of a **texture buffer** (`GL_TEXTURE_BUFFER`, core in GL 3.1):

   | texel | contents |
   |---|---|
   | 0–2 | rows 0–2 of the inverse model matrix |
   | 3 | colour.rgb, primitive type |
   | 4 | ka, kd, ks, shininess |
   | 5 | emissive.rgb, opacity |
   | 6 | reflectivity, texture slot, uvScale.xy |
   | 7 | unlit flag |

4. Upload group spheres and index ranges as uniform arrays (max 32 groups), plus lights (the same
   `uLights[]` uniforms as the rasteriser, uploaded by the same function) and a per-light shadow flag.

Rebuilding every frame keeps the ray tracer live: animation, driving and editing all show up immediately.
Nothing is allocated per frame (vectors are reused).

## 4. The per-pixel algorithm (`raytrace.frag`)

A full-screen triangle (generated from `gl_VertexID`, no vertex buffer) runs the fragment shader once per
pixel of a reduced-resolution framebuffer.

```
ray = camera position, normalize(forward + x·tan(fov/2)·aspect·right + y·tan(fov/2)·up)
colour = 0, throughput = 1
repeat up to (bounces + 1) times:
    hit = nearest intersection over all groups whose bounding sphere the ray hits
    if no hit: colour += throughput · background; stop
    P = hit point, N = world normal (flipped to face the ray)
    albedo = colour × texture(uv from the analytic hit point)
    if unlit: shaded = emissive + albedo
    else:
        for each enabled light:
            L, attenuation, spot factor               (lightVector() from lighting.glsl)
            visible = light casts shadows ? !occluded(P + ε·N, L, distance to light) : true
            accumulate diffuse + specular × visible     (addLight() from lighting.glsl)
        shaded = albedo·(ka·Ia + diffuse) + specular + emissive
    if opacity < 1:     colour += throughput·opacity·shaded; throughput ·= (1 − opacity); continue straight on
    else:               colour += throughput·(1 − reflectivity)·shaded
                        if reflectivity = 0: stop
                        throughput ·= reflectivity; ray = reflect(ray, N)       (mirror bounce)
    stop early when throughput < 2 %
```

* **Shadow rays** (`occluded`) are *any-hit* tests with a maximum distance; light sources (unlit parts such
  as the bulb, sky, sun) and transparent glass do not cast shadows.
* The small offset `ε·N` (2·10⁻³) avoids a surface shadowing itself ("shadow acne").
* The **floor** (reflectivity 0.18), **car paint** (0.15), **car glass** (0.3), **ball** (0.12) and
  **Buzz's helmet** (glass: opacity 0.22, reflectivity 0.25) show reflections and transparency.
* Sunlight enters **only through the window opening** — the walls now block it, which rasterisation cannot
  do.

## 5. Output

The traced image (default 50 % resolution) is stored in a colour texture attached to a framebuffer object,
then drawn over the whole window with bilinear filtering (`present.frag`). A blit cannot be used because
the window's framebuffer is multisampled (4× MSAA).

## 6. Acceleration and cost

* Per pixel: primary ray + (per bounce) one shadow ray per shadow-casting light.
* Bounding spheres per object skip most parts for most rays (a ray that misses Bullseye's sphere never
  tests his ~30 parts).
* Resolution scale and bounce count are adjustable at run time to trade quality for speed; the title bar
  shows the number of primitives and the scale.

## 7. Mouse picking uses the same maths on the CPU

`RayIntersect::Object(type, inverse(model), ray)` in `src/math/Ray.cpp` implements the identical
intersection routines in C++. Clicking the window casts one ray and selects the nearest owner
([05](05-camera.md#5-mouse-picking--the-inverse-of-projection)).
