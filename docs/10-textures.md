# 10 — Textures

Files: `src/render/ProceduralTextures.*`, `src/render/BmpLoader.*`, `src/render/Image.h`,
`src/gl/Texture.*`, `src/render/Assets.cpp`, `tools/make_poster.py`.

No image library is used. Textures come from two sources that we write ourselves:

1. **Procedural textures** — every texel is computed by a formula in C++.
2. **BMP files** — read by our own BMP parser (the poster), and written by our own BMP writer (F12
   screenshots).

## 1. Texture coordinates (UV mapping)

Every vertex carries a `uv` in [0, 1]²: u runs left → right across the image, v bottom → top.
The rasteriser interpolates uv across each triangle and the fragment shader samples:

```glsl
vec3 texColor = texture(uTexture, vUV * uMaterial.uvScale).rgb;
vec3 albedo   = uMaterial.color * texColor;          // colour × texture (tinting)
```

`uvScale` repeats the texture: the floor uses (8, 4), so the 4-plank image repeats 8 × 4 times over the
20 × 18 floor (one repeat = 2.5 × 4.5 units), keeping planks the right size. With `GL_REPEAT`, a
coordinate u > 1 wraps to `u − floor(u)`.

**Textures are off by default** (Settings panel or **F3** turns them on). A material can define a
`plainColor` used while textures are off (the wood floor becomes a flat brown, the wallpaper a flat blue);
otherwise its colour is used on its own. Because the colour multiplies the texture, a greyscale texture is
*tinted* by the material colour: one fabric image serves as Woody's yellow plaid shirt, Jessie's white
shirt, Bullseye's brown coat and Penny's fur.

| Primitive | UV mapping ([03](03-primitives.md)) |
|---|---|
| Plane | u = x + 0.5, v = 0.5 − z (the image lies flat, "up" = −Z) |
| Cube | each face gets the whole image, (0,0) at its bottom-left seen from outside |
| Sphere | u = longitude / 360°, v = latitude mapped to [0, 1] (equirectangular) |
| Cylinder | side: u = angle / 360°, v = height; caps: disk mapped onto the image |
| Cone | side like the cylinder, apex at v = 1; base cap like a disk |

Walls are rotated planes whose local X = wall width and local Z = wall height, so wallpaper stripes stand
upright on every wall (see `AddWall` in `Room.cpp`).

## 2. From image to OpenGL texture (`Texture` class)

```cpp
glGenTextures(1, &id);
glBindTexture(GL_TEXTURE_2D, id);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);              // uv > 1 tiles
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);          // bilinear
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
glGenerateMipmap(GL_TEXTURE_2D);
```

* **Magnification** (texture seen closer than 1 texel per pixel): bilinear interpolation of the 4 nearest
  texels.
* **Minification** (far away, many texels per pixel): **mipmaps** — pre-shrunk copies at ½, ¼, … size.
  Trilinear filtering picks the two nearest mip levels and blends them. Without it the distant floor planks
  shimmer (aliasing).
* **Bilinear filtering.** For texture coordinates (u, v) on a W × H image, the sample position is
  `(x, y) = (u·W − ½, v·H − ½)`; with `i = floor(x)`, `j = floor(y)`, `fx = x − i`, `fy = y − j`:
  ```
  colour = (1−fx)(1−fy)·T[i, j] + fx(1−fy)·T[i+1, j] + (1−fx)fy·T[i, j+1] + fx·fy·T[i+1, j+1]
  ```
* **Mip level selection.** Level k is the image downsampled k times by 2 (each texel = average of 2 × 2
  texels of level k−1). The GPU estimates how many texels one pixel covers from the screen-space
  derivatives of the texture coordinates,
  `ρ = max( |∂(uW, vH)/∂x|, |∂(uW, vH)/∂y| )`, and uses level `λ = log₂ ρ`. Trilinear filtering samples the
  two levels ⌊λ⌋ and ⌈λ⌉ bilinearly and blends them by the fraction of λ. A 512² floor texture has
  10 levels (512 … 1); the mip chain adds one third to its memory (1 + ¼ + 1/16 + … = 4/3).
* **Texture units:** the rasteriser binds the material's texture to unit 0 (`uTexture = 0`). Untextured
  materials bind a 1 × 1 white texture, so `colour × white = colour` and the shader needs no branch.

**F3** toggles texturing on/off (materials fall back to their plain colours).

## 3. Procedural textures (`ProceduralTextures.cpp`)

All generators evaluate a function `f(u, v) → colour` at the centre of every texel
(`u = (x + 0.5) / width`).

### Noise
* `Hash(x, y) = fract(sin(127.1x + 311.7y) · 43758.5453)` — a pseudo-random number per lattice point.
* `ValueNoise(x, y)` — random values at integer lattice points, blended with the smoothstep curve
  `3t² − 2t³` → smooth, continuous randomness.
* `Fbm` (fractal Brownian motion) — sum of 4 octaves of noise, each with double frequency and half amplitude.

### The textures

| Texture | Formula (short) | Used by |
|---|---|---|
| Wood floor 512² | 4 planks across u; each plank split into 2 boards along v with a random offset; grain = `sin((10·u + 6·fbm) · π)` distorted rings; per-board tint `0.85 + 0.3·hash`; dark 2 % gaps at edges | floor, desk |
| Wallpaper 256² | 8 vertical stripes of two blues; a diamond `|x| + |y| < 0.18` in every other stripe; subtle noise | walls |
| Rug 256² | square rings `max(|x|, |y|)` quantised into 4 colour bands + fibre noise | rug |
| Beach ball 256×128 | u (longitude) split into 6 coloured gores; white caps where v < 0.08 or v > 0.92 (the poles) | ball |
| Toy block 128² | dark bevel where `max(|x|,|y|) > 0.44`; five-pointed star where `r < 0.18 + 0.09·cos(5θ − 90°)` | toy blocks (tinted by material colour) |
| Night sky 1024×512 | 64 × 32 cells, ~45 % hold a star at a random position with random size/brightness | sky behind the window |
| Checker | `(floor(u·n) + floor(v·n)) mod 2` | available helper |
| Moon 1024×512 | see below | moon (unlit, colour 1.65 × texture) |
| Fabric ×5, 256² | `shade = 0.78 + 0.15·noise(120u, 120v) + 0.07·weave`, `weave = ½ + ½ sin(256πu)·sin(256πv)`; denim × `0.8 + 0.2|sin(160(u+v))|` (twill); leather `0.65 + 0.3·fbm(45u, 45v) + 0.1·noise`; plaid: brown lines where `|sin(8πu)| < 0.18` or `|sin(8πv)| < 0.18`; cow print: black where `fbm(9u, 9v) > 0.51` | clothes, saddle, hats, boots, belts, Bullseye's coat and mane, Jessie's braid, the bed sheet, the tyres, Penny's fur |
| Siding 256² | 8 boards along v; in a board `b = frac(8v)`: `shade = 0.80 + 0.16·b`, × `(0.55 + 0.45·b/0.08)` in the shadow line `b < 0.08`, × grain `0.94 + 0.06·fbm(6u, 90v)` | house walls, gables, garage door (tinted yellow / grey) |
| Shingles 256² | 8 rows × 6 tabs, odd rows shifted by half a tab; each tab `0.72 + 0.22·hash(tab, row)`, lighter towards its exposed edge, dark gaps | roofs (tinted terracotta) |
| Brick 256² | running bond: 8 courses × 4 bricks, odd courses offset by ½; mortar where `frac(8v) < 0.12` or `frac(4u + offset) < 0.04`; each brick `0.75 + 0.25·hash` | foundation, column bases, chimney |
| Grass 256² | `0.62 + 0.30·fbm(8u, 8v) + 0.18·(noise(180u, 60v) − ½)` — clumps plus fine blades | the lawn (repeated 80 × 80) |
| Book spines 256² | 12 books across u; book k has height `h = 0.72 + 0.26·hash(k)` (the dark shelf back shows above it), a palette colour, a gold title band at `h − 0.12`, rounded shading `0.75 + 0.25 sin(π·u_book)` and dark gaps | the 4 book boxes (ray-tracer slot 13) |
| Pickets 256², RGBA | 8 pickets across u: picket where `|frac(8u) − ½| < 0.22` and below a triangular tip `v < 0.80 + 0.14(1 − off/0.22)`; rails at v ∈ [0.24, 0.33] and [0.60, 0.69]; **alpha = 1 on pickets and rails, 0 elsewhere** | fence and porch railing cut-outs (slot 14) |
| Window pane 128² | white frame where u or v is within 0.06 of the edge, cross mullions where `|u − ½| < 0.03` or `|v − ½| < 0.03`; glass blue graded with v plus a diagonal sheen | house windows (raster only) |
| Flower bed 256² | leafy green noise; a 10 × 10 grid of cells each holding one blossom (radius 0.28 of a cell, random position and one of 4 colours) | the flower box (raster only) |

**The moon.** For texel (u, v) the shader-like generator first converts the texel to a direction on the
unit sphere — `longitude = 2πu`, `latitude = (v − ½)π`,
`n = (cos lat · sin lon, sin lat, cos lat · cos lon)` — so the pattern is continuous across the texture
seam and does not pinch at the poles. Then:

```
shade = 0.40 + 0.60 · fbm(5 nx + 3 nz + 12, 7 ny + 4)                 dark maria and bright highlands
      + 0.10 · (noise(180 nx + 40 nz, 180 ny) − ½)                     fine grain
for each of 110 craters (centre c on the sphere, radius r = 0.018 + 0.17·hash²):
      d = |n − c| / r          (|n − c|² = 2 − 2 n·c, cheap)
      shade −= 0.18 · exp(−3 d²)                                       bowl
      shade += 0.20 · exp(−70 (d − 1)²)                                bright rim at d = 1
      shade += 0.16 · (ny − cy)/r · exp(−2 d²)                         one side lit, one in shadow
lit   = 0.32 + 0.68 · max(0, n · normalize(−0.15, 0.25, 1))            baked sunlight with a soft terminator
colour = (0.94, 0.95, 1.0) · shade · lit
```

### Texture instead of geometry

Fine surface detail used to be built from extra shapes: 48 hat-stitch spheres, 72 tyre-tread cubes,
12 braid beads, 14 saddle studs, 9 mane strands, quilt seams and 24 book spine bands. Every shape costs a
draw call, a matrix and, for a sphere, up to 1 656 triangles, every frame. A texture costs one lookup per
pixel no matter how much detail it shows, and it is filtered (mipmaps) so it never flickers in the
distance. These details are now textures (`denim` on the tyres, `woven cotton` on the braid and mane,
`leather` on the hat band and saddle, `plaid` on the bed). Shapes are kept only where they change the
**silhouette** (ears, the scarf, the hat brim). See [17 — Performance](17-performance.md).

A second round replaced: the 24 book cubes (→ 4 boxes with the book-spine texture), the fence's ~50 picket
cubes and 4 rails and the porch's ~12 balusters and rails (→ 4 cut-out boxes), the 3 extra cubes of every
window (→ the window-pane texture), the 7 flower spheres (→ one textured flower bed), plus small painted-on
details that are now simply left to the clothing textures (shirt buttons, pockets, stripes, handles,
buckles, seal rings).

### Alpha cut-outs

A picket fence is mostly holes. Instead of modelling every picket, one thin box carries the **Pickets**
texture whose alpha channel is 1 on the wood and 0 in the gaps. The fragment shader discards the gaps:

```glsl
vec4 texel = texture(uTexture, vUV * uMaterial.uvScale);
if (uCutout == 1 && texel.a < 0.5) discard;
```

`uvScale.x = length / (8 · spacing)` keeps the picket spacing constant whatever the panel's length (8
pickets per texture repeat): 0.7 units for the fence, 0.4 for the porch balusters. The alpha describes the
object's *shape*, so the texture is bound even when textures are switched off (only its colour is then
ignored). Cut-outs are left out of the lamp's shadow map. In the ray tracer the same test rejects hits in
the gaps ([12 §6](12-ray-tracing.md)), so light passes between the pickets.

*Worked example, the left fence panel.* It runs from x = −9.95 to x = 11.0, length L = 20.95, so
`uvScale.x = 20.95 / (8 · 0.7) = 3.741` repeats. On the cube's front face `u = x_local + ½` runs 0 … 1 across
the panel, so the texture coordinate is `u' = 3.741·(x − (−9.95))/20.95`. At x = −9.6 (0.35 from the end)
`u' = 0.0625`, `frac(8u') = 0.5`: the centre of the first picket (alpha 1). At x = −9.25 (0.70 from the end)
`frac(8u') = 0.0`: the middle of a gap (alpha 0, discarded). The porch railing, 2 units long with 0.4
spacing, uses `uvScale.x = 2 / 3.2 = 0.625`.

Mipmapping averages alpha too: far away, pickets and gaps blend to alpha ≈ 0.5 and the fence thins out.
That is acceptable here; the standard fixes are alpha-to-coverage or scaling alpha per mip level.

The **star texture** is used additively: sky output = `emissive (sky colour) + colour × stars`. By night the
material colour is white (stars visible); by day it fades to black, so the stars disappear into the blue sky
without swapping textures.

## 4. Our BMP loader (`BmpLoader.cpp`)

BMP layout (little-endian):

| Offset | Size | Field |
|---|---|---|
| 0 | 2 | "BM" signature |
| 2 | 4 | file size |
| 10 | 4 | offset of pixel data |
| 14 | 4 | info header size (40) |
| 18 | 4 | width |
| 22 | 4 | height (positive = rows stored **bottom-up**, negative = top-down) |
| 26 | 2 | planes (1) |
| 28 | 2 | bits per pixel (24 or 32 supported) |
| 30 | 4 | compression (0 = BI_RGB, 3 = BITFIELDS) |

Pixel rows are stored as **B, G, R (, A)** and each row is padded to a multiple of 4 bytes:
`rowSize = (width · bytesPerPixel + 3) & ~3`.

The loader validates the header, swaps BGR → RGB, handles bottom-up and top-down files and fills an
RGBA `Image`. Bottom-up BMP rows match OpenGL's convention that texture row 0 is at v = 0, so no flip is
needed for normal files.

`Bmp::Save` writes the same format (24-bit, bottom-up); **F12** uses it with `glReadPixels` to save
screenshots into `screenshots/shot_NNN.bmp`.

### The poster asset
`tools/make_poster.py` draws `assets/textures/poster.bmp` pixel by pixel in pure Python (gradient space
background, stars, ringed planet, rocket, "TO INFINITY AND BEYOND" with a 5 × 7 bitmap font) and writes it
as a 24-bit BMP with the same header layout. If the file is missing the program falls back to a procedural
poster.

## 5. Textures in the ray tracer

The ray tracer computes UV analytically from the object-space hit point using formulas matching the primitive mesh. All 22 mapped surfaces, including the six exterior maps, occupy layers of one 512-by-512 texture array. Source maps are bilinearly resampled, mipmaps are generated, and a runtime layer index is legal with sampler2DArray in GLSL 3.30. This uses one surface-map binding instead of 15 individually selected samplers. Cutout alpha is sampled at level zero so neighbouring rays on unrelated surfaces do not blur coverage holes. The source `.rgba` exports retain alpha; BMP exports are colour previews.
