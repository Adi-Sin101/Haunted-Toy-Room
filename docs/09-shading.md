# 09 — Shading Models (Lecture 9)

Illumination ([08](08-illumination.md)) says *how much light a point reflects*. Shading says *where we
evaluate that formula* — once per triangle, once per vertex, or once per pixel. Press **F2** to cycle
through the four models live; the title bar shows the active one.

| Mode (F2) | Normal used | Where lighting is computed | Shader files |
|---|---|---|---|
| **Flat** | one per triangle (face normal) | per fragment, but constant over the face | `lit.vert` + `lit.frag` (`uShadingMode = 0`) |
| **Gouraud** | vertex normals | **per vertex**, colours interpolated | `gouraud.vert` + `gouraud.frag` |
| **Phong** | vertex normals interpolated per pixel | **per pixel**, Phong specular (R·V) | `lit.vert` + `lit.frag` (`uShadingMode = 2`, `uBlinn = 0`) |
| **Blinn-Phong** (default) | as Phong | per pixel, half-vector specular (N·H) | `lit.frag` (`uBlinn = 1`) |

## 1. Flat shading

Every triangle gets a single normal, so each facet has one uniform brightness — the sphere looks like a
faceted gem, which makes the triangle structure of our meshes visible.

Instead of storing duplicate vertices with face normals, the fragment shader **reconstructs the face
normal** from screen-space derivatives of the world position:

```glsl
N = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
```

`dFdx` / `dFdy` give how the world position changes between neighbouring pixels in x and y. Both vectors
lie in the triangle's plane, so their cross product is perpendicular to the triangle, i.e. its face normal.
(It automatically faces the camera, so it is never flipped for back faces.)

![Flat shading with vertex normals (F9) on Jessie](images/flat-normals.png)

*Flat shading (F2) with the selected object's vertex normals shown (F9). Every facet has one brightness.*

## 2. Gouraud shading

The full illumination model runs in the **vertex shader** (`gouraud.vert`):

```glsl
computeLighting(worldPos, N, V, diffuse, specular);   // per vertex
vLight    = ambientTerm() + diffuse;                  // interpolated by the rasteriser
vSpecular = specular;
```

The fragment shader only combines the interpolated values with the colour/texture:
`color = albedo * vLight + vSpecular + emissive`.

* Cost: lighting once per vertex (627 times for a sphere instead of once per pixel).
* Artefacts to look for: specular highlights look blotchy / star-shaped on coarse meshes and can vanish
  entirely when the highlight falls between vertices (look at the ball, the car roof, the floor under the
  lamp spot — the lamp's pool of light on the big floor plane almost disappears because the floor has only
  four vertices!). This is the classic Gouraud weakness.

![Gouraud at night](images/gouraud-night.png)

*Gouraud at night: the lamp's spot light never reaches the floor's four vertices, so the pool of light on the floor is missing. Compare with the Phong/Blinn images in [00](00-index.md).*

## 3. Phong shading

The **normal** is interpolated across the triangle (vertex shader outputs `vNormal`), re-normalised per
fragment, and the illumination model is evaluated **per pixel**:

```glsl
vec3 N = normalize(vNormal);                 // interpolated normal, unit length again
vec3 V = normalize(uCameraPos - vWorldPos);
computeLighting(vWorldPos, N, V, diffuse, specular);
```

Smooth surfaces look smooth even with few triangles, highlights are round and sharp, and the lamp's spot
light forms a correct pool on the floor even though the floor is only two triangles.

## 4. Blinn-Phong

Same as Phong, only the specular term uses the half vector `H = normalize(L + V)` and `N·H` instead of
`R·V` (see [08](08-illumination.md)). It is the default mode.

## 5. Interpolation in detail

The rasteriser computes, for each pixel inside a triangle, barycentric weights (α, β, γ) with
α + β + γ = 1 and interpolates each vertex output:

```
value(pixel) = α·value(v0) + β·value(v1) + γ·value(v2)       (perspective-correct in OpenGL)
```

* Gouraud interpolates **colours** → lighting detail smaller than a triangle is lost.
* Phong interpolates **normals** (and positions) → lighting is recomputed per pixel.

## 6. The normal matrix again

All modes transform normals with `uNormalMatrix = transpose(inverse(mat3(model)))`
([04](04-transformations.md#5-transforming-normals--the-normal-matrix)). Try: select Bullseye, Tab, T → Scale,
stretch him on one axis — lighting stays correct in every shading mode.

## 7. What to show the teacher

1. Select the ball (6) or Woody (1), press **F** to orbit close, **F2** to cycle modes.
2. **F1** wireframe shows the actual triangles that Flat shading makes visible.
3. **F9** shows the vertex normals used by Gouraud/Phong (cyan lines), **F10** the vertices themselves.
4. Night (`,` / `.` to scrub, or start at night) + lamp: compare the lamp's spot on the floor in Gouraud
   (almost missing) vs Phong (clean circle).
