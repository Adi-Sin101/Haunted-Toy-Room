# Graphics requirements and demonstration evidence

Adiba Tahsin | Roll 2107031 | CSE-4102

| Requirement | Implemented behaviour and source | Report location | Demonstration |
| --- | --- | --- | --- |
| Lighting | Eight light slots, including directional, point and spot; shared ambient/diffuse/specular illumination in `shaders/lighting.glsl` | 2.5, 4.1 | Move lamp (7, A/D, W/S), switch power with R; F5/F6/F7 isolate terms |
| Transformations | Translation, rotation, scale, shear, reflection, inverse-transpose normals and parent-child world matrices in `src/math/Transform3D.cpp` and `src/scene/SceneNode.cpp` | 2.2-2.4, 3.3 | Tab, T, J/L, U/O, I/K; M mirrors; Jessie mounts with R |
| Gouraud shading | Per-vertex ambient/diffuse and specular values, perspective-correct interpolation in `gouraud.vert` / `gouraud.frag` | 2.6, 4.2 | F2 chooses raster mode and cycles shading; matched figure |
| Phong shading | Interpolated, renormalised normal and per-fragment lighting in `lit.frag` | 2.5-2.6, 4.2 | F2; compare narrow highlights with Gouraud |
| Motion / animation | Gaits, flight, distance-driven wheel and ball rotation, fan, clock, hinges, Penny's route and seven mission stages | 3.2-3.11, 4.5 | Automatic replay, Buzz flight, mounting and car return; O enables haunted props |
| Interaction | Keyboard, mouse ray picking, object inspector, movement/special controls, live actor ownership and full manual mode | 2.4, 4.5 | Select a toy while others continue; 0 releases it; N selects full manual; V prints parts |
| Ray tracing bonus | GPU analytic plane/cube/sphere/cylinder/cone intersections, median-split BVH, shadow queries, bounded mirror/transmission paths in `RayTracer.cpp` / `raytrace.frag` | 2.8-2.9, 4.4 | F4; 9 cycles 0-4 continuations; inspect polished-floor reflection |
| Textures | 21 procedural/BMP surface maps, matching primitive UVs, filtering, mipmaps, cutout alpha and ray texture array in `Assets.cpp` | 2.7, 4.3 | F3; all-map atlas and construction catalogue |
| Proposal coverage | Complete toy room, primitive cast, independent objects, rider hierarchy, night/morning sequence and detailed environment | 1.3 | Proposal-to-implementation table; full mission reaches The End |
| Presentation | Introduction, actual two-minute demo, feature screenshots with descriptions, thank-you; ten slides and speaker notes | Presentation / study guide | Slide 2 embeds the same MP4 as the standalone demo |
| Detailed report | Combined cover with KUET logo, theory, formulas, actual images, implementation, results and references; four chapters | Entire LaTeX report | Canonical PDF and editable `.tex` source |
| Code understanding | Full vertex/index examples, light and ray colour calculations, explicit bounce budget, source route and parameter exercises | 2.1, 2.5, 2.8; study guide | Shift+V geometry dump; shininess, light cutoff, UV repeat and ray-budget changes |

The ray tracer starts with one primary hit and permits two continuations by default: at most three surface evaluations. The configurable range is zero to four continuations. Reflection weights the next path by reflectivity; straight transmission weights it by one minus opacity. The final permitted hit contributes local illumination, and weak throughput terminates early. Shadow rays test visibility and do not count as additional colour continuations. These details match the shader, including the absence of physical refraction and diffuse indirect lighting.

The report explains every distinct object group. `Comprehensive-Implementation-Notes.md`, the numbered source documentation and `inventory/objects.csv` preserve every individual node and local transform; `inventory/materials.csv` records every coefficient and mapped layer. Thus the page limit does not remove the exhaustive implementation records.

Evidence: Debug and Release build logs; 36 checks; real manual/mounted/live key-callback scenarios; five live-owned mission replays; a matched unaffected-route comparison; 42 captures; full arrival/mission completion; and decoded 120-second video. `validation/final-validation.json` is the consolidated record.
