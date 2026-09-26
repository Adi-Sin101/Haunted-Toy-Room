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

`uvScale` repeats the texture: the floor uses (4, 3), so the 4-plank image repeats 4 × 3 times over the
16 × 12 floor, keeping planks the right size.

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

The ray tracer computes uv analytically from the object-space hit point with formulas that match the mesh
UVs exactly (`primitiveUV` in `raytrace.frag`), then samples one of 7 fixed texture slots
(`Assets::TextureSlot`: floor, wall, rug, ball, block, poster, stars). GLSL 3.30 cannot index an array of
samplers with a run-time value, so each slot is a separate uniform and the shader selects with `if`.
