# 17 - Performance

The scene used to run at about 2 frames per second in the Debug build. This chapter records what was
measured, what caused it, and what changed. All numbers come from `--benchmark`, which turns v-sync
off, skips 60 warm-up frames and averages the next 200–400 frames of the story (1600 x 900, AMD Radeon
integrated graphics).

## Measurements

| | Before | After |
|---|---|---|
| Debug build, default settings | 420 ms per frame (2.4 FPS) | 4.8 ms (207 FPS) |
| Debug build, lighting + Blinn-Phong + textures | 441 ms (2.3 FPS) | 4.8 ms (209 FPS) |
| Release build, lighting + Blinn-Phong + textures | 4.1 ms (243 FPS) | 3.4 ms (291 FPS) |
| Triangles per frame (lighting on, shadow pass included) | 1,280,000 | 44,000 – 49,000 |
| Draw calls per frame (lighting on, shadow pass included) | 1,340 | 420 – 440 |

The window title shows the live draw-call and triangle counts. (These first measurements were taken
when the optimisation was made; the machine was running faster at that time than later in the day.)

### After adding the house and Penny

Measurements vary with the machine's state (power mode, temperature), so the house was measured against
the previous version **in the same session**, both built from source:

| | Before the house | With the house and Penny |
|---|---|---|
| Debug build, default settings, story | 7.6 ms | 8.1 ms |
| Release build, lighting + Blinn-Phong + textures, story | 3.5 ms | 3.8 ms |
| Release build, Penny's arrival (house visible) | — | 1.8 ms default / 2.4 ms with lighting |
| Ray tracing, story / arrival | — | 9.3 ms / 19.0 ms |

The ~6–8 % increase during the story is Penny (34 shapes) and the three doors. The house itself costs
nothing once Penny is inside, because of the visibility switching described in section 5.

## 1. Physics: a feedback loop (the 2 FPS problem)

Timing each stage of a Debug frame showed that `PhysicsWorld::Update` took 456 of the 480 ms.
The block simulation runs in fixed 1/120 s steps. In each step, every block was tested against every
piece of furniture in six separation passes. Each test also rebuilt the furniture's world-space box
from its matrix. A slow frame produced a large time step, which needed more steps, which made the next
frame slower still.

Fixes in `PhysicsWorld::Update`:

- Furniture does not move while blocks are simulated, so its boxes are computed **once per frame**.
  `Move()` and `FireLaser()` use the same cached boxes.
- **Broad phase:** each step, every block collects the few pieces of furniture within 0.25 units of
  it. The separation passes then test only those.
- A block's rotation is fixed during separation, so its half-size is computed once per step instead
  of once per contact test.
- At most **6 steps per frame**, so a slow frame cannot cause even more work in the next one.

The 19 checks in `tests/PhysicsChecks.cpp` still pass.

## 2. Geometry: too many triangles for the detail they show

Every sphere used one 32 x 48 mesh, which is 2,976 triangles. The same mesh was used for a 5 cm eye
highlight and for the moon. Some surface detail was also built from extra shapes that a texture
shows better:

| Removed geometry | Shapes | Replaced by |
|---|---|---|
| Hat stitching (Woody, Jessie) | 48 spheres | leather texture on the hat band |
| Jessie's braid beads | 12 spheres | woven texture on one plait cylinder |
| Saddle studs | 14 spheres | leather texture on the saddle |
| Bullseye's mane strands | 9 cylinders | woven texture on the mane |
| Tyre tread blocks, radial spokes | 72 + 16 cubes | twill texture on the tyre, one cross spoke |
| Car grille bars | 7 cubes | one grille block |
| Quilt seams, book spine bands | 9 + 24 cubes | quilt fabric texture, plain covers |
| Buzz's wing stripes, chest vent slats | 10 + 5 cubes | red wing tips, one vent panel |
| Trouser seams | 6 cylinders | denim texture |
| Curtain folds | 14 cylinders | 8 wider textured folds |

### Level of detail

Spheres, cylinders and cones now have three meshes each (`Assets::Load`):

| Shape | Full | Medium | Low |
|---|---|---|---|
| Sphere (stacks x sectors) | 24 x 36 = 1,656 triangles | 12 x 18 = 396 | 6 x 10 = 100 |
| Cylinder / cone (sectors) | 32 | 16 | 8 |

For each shape, `Renderer::CollectNode` computes its bounding radius divided by its distance to the camera,
which gives its approximate size on screen. `Mesh::ForScreenSize` picks the full mesh only when that
value is above 0.06. A button or a distant toy therefore gets the 100-triangle sphere, while a
close-up head still gets the smooth one. The ray tracer is unaffected: it intersects the exact
analytic shapes.

## 3. Rendering: fewer and cheaper draw calls

- **Frustum culling:** each shape's bounding sphere is tested against the six planes taken from the
  view-projection matrix. Shapes that are off-screen or behind the camera are never submitted.
- **Lamp shadow pass:** it runs only while the lamp is on. It skips shapes outside the lamp's cone
  and shapes smaller than a shadow-map texel, and it uses the level of detail as seen from the lamp.
- **Sorted draws:** opaque shapes are sorted by material, then by mesh. Material uniforms, the
  texture binding and the VAO change once per group instead of on almost every draw.
- **Cached uniform locations:** the per-draw uniforms (`uModel`, `uNormalMatrix`, the material
  fields) are looked up once at start-up instead of by name on every draw call.

## 4. Scene graph: one matrix per node, built directly

`Transform::Matrix()` used to multiply five 4 x 4 matrices (T, Ry, Rx, Rz, S) for every node every
frame. It now writes the rotation `Ry * Rx * Rz` directly from the sines and cosines, scales its
columns and adds the translation. The result is the same matrix (checked against the original product
on 2,000 random transforms, including shear), at a fraction of the cost.

## 5. The house: hide what cannot be seen

The house adds about 220 shapes (facade, roof, porch, garage, garden, fence, trees, ground-floor corridor,
stairs). Two observations make them free during the story:

* **Nothing outside is visible from inside.** `HouseExterior.visible` turns off when Penny reaches the
  stairs.
* **Nothing downstairs is visible from upstairs once the stair door is shut.** `HouseInterior.visible`
  turns off when the arrival ends.

Before the stair door existed, the ground floor's large walls were inside the camera's view (behind the
room's walls) and were shaded every frame before being overwritten — **overdraw** — and in ray tracing
they were tested by every ray, because unowned scenery is in the always-tested group. Measured
back to back, hiding the ground floor took lit Release from 4.7 ms to 4.2 ms and ray tracing from
11.6 ms to 9.6 ms.

A hidden node is skipped by `SceneNode::UpdateWorld` as well as by the draw list, so hidden subtrees
cost no matrix work either. This is a simple form of **occlusion culling** that uses knowledge of the
scene (a closed door) instead of a general visibility algorithm.

## Re-running the measurements

```
bin\Release\HauntedToyRoom.exe --benchmark 400
bin\Release\HauntedToyRoom.exe --benchmark 400 --lighting --shading 3 --textures
bin\Debug\HauntedToyRoom.exe --benchmark 200
bin\Release\HauntedToyRoom.exe --no-intro --benchmark 400 --raytrace
```

`--no-intro` measures the story only (without Penny's arrival). The benchmark also prints, for every
object, its shape count and the triangles it would cost at full detail.
