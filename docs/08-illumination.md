# 08 — Illumination (Lecture 8)

Files: `shaders/lighting.glsl` (the model, shared by every shader), `src/scene/Light.h`,
`src/scene/Material.h`, `Renderer::UploadLights`, `ToyRoomApp::UpdateLights`, `src/world/Environment.cpp`.

## 1. The Phong illumination model

For a surface point P with unit normal N, unit vector to the viewer V, and for each light i the unit vector
to the light Lᵢ:

```
I = ka·Ia·C                                            ambient
  + Σᵢ  fatt(dᵢ) · spotᵢ · Ilᵢ · [ kd·C·max(N·Lᵢ, 0)    diffuse  (Lambert's cosine law)
                                + ks·max(Rᵢ·V, 0)ⁿ ]   specular (Phong)
  + E                                                  emission
```

| Symbol | Meaning | In code |
|---|---|---|
| C | object colour (diffuse reflectance) × texture colour | `uMaterial.color * texture(...)` |
| ka, kd, ks | ambient / diffuse / specular reflection coefficients | `Material::ka, kd, ks` |
| n | specular exponent (shininess) | `Material::shininess` |
| Ia | ambient light intensity (colour) | `uAmbientLight` from `Environment::AmbientLight()` |
| Il | light source intensity (colour × intensity) | `uLights[i].color * uLights[i].intensity` |
| Rᵢ | mirror reflection of Lᵢ about N: `R = 2(N·L)N − L` = GLSL `reflect(−L, N)` | |
| fatt | distance attenuation | see below |
| E | emissive colour (bulb, laser, sky, sun, moon, headlight bulbs) | `Material::emissive` |

The shader computes diffuse and specular *light* first and multiplies by the colour at the end:

```glsl
color = albedo * (ambientTerm() + diffuse) + specular + uMaterial.emissive;
```

Specular highlights are **not** tinted by the object colour — they are the colour of the light, as on a
real plastic toy.

### Diffuse (Lambert)
A surface receives light proportional to the cosine of the angle between its normal and the light
direction: `max(N·L, 0)`. Faces turned away (N·L < 0) get no direct light.

### Specular (Phong) and Blinn-Phong
* **Phong:** `max(R·V, 0)ⁿ` — bright when the viewer is near the mirror direction.
* **Blinn-Phong:** `max(N·H, 0)^(4n)` with the half vector `H = normalize(L + V)`. Cheaper and better
  behaved at grazing angles; the exponent is multiplied by 4 so the highlight has a similar size.
  Choose with **F2** (see [09](09-shading.md)).

The specular term is only added where `N·L > 0` (no highlight on the dark side).

## 2. Light source types (`LightType`)

| Type | Direction L at P | Attenuation | Used for |
|---|---|---|---|
| Directional | constant: `L = −light.direction` | none (infinitely far) | sun (day), moon (night) |
| Point | `normalize(light.position − P)` | `1 / (kc + kl·d + kq·d²)` | lamp bulb glow, Buzz's laser glow, ghost glow |
| Spot | as point | as point × cone factor | lamp shade cone, RC car headlights |

**Spot cone.** With θ = angle between the spot axis and the ray light → P:

```
spot = smoothstep(cos(outer), cos(inner), cos θ)     1 inside the inner cone, 0 outside the outer cone,
                                                     smooth fall-off in between (soft edge)
```

Lamp spot: inner 22°, outer 34°. Headlights: inner 12°, outer 22°.

**Attenuation** constants used (kc, kl, kq):

| Light | kc | kl | kq |
|---|---|---|---|
| lamp bulb | 1 | 0.14 | 0.07 |
| lamp spot | 1 | 0.05 | 0.01 |
| headlights | 1 | 0.10 | 0.05 |
| laser glow | 1 | 0.35 | 0.40 |
| ghost glow | 1 | 0.30 | 0.15 |

## 3. The lights in the scene (`ToyRoomApp` light slots)

| Slot | Light | Position / direction comes from | Casts ray-traced shadows |
|---|---|---|---|
| 0 | Sun / Moon (directional) | `Environment::UpdateSky` — from the sun or moon sphere toward the room centre | yes |
| 1 | Lamp bulb (point) | lamp `LightAnchor` node | yes |
| 2 | Lamp spot | `LightAnchor` position, head joint's −Y axis | yes |
| 3, 4 | Car headlights (spot) | car `HeadlightL/R` nodes, car's +Z axis tilted down | no |
| 5 | Laser glow (point, red) | Buzz's `LaserTip` node, only while the laser is on | no |
| 6 | Ghost glow (point, blue) | ghost node, fades with the ghost | no |

Because lights read their placement from scene nodes every frame, they follow the hierarchy: tilt the
lamp head (select 7, W/S) and the spot cone moves; drive the car and the headlight beams sweep the room.

Up to 8 lights (`MaxLights`) are uploaded as the uniform array `uLights[i]` with a `uLightCount`.

## 4. Day and night lighting

`Environment` changes the lighting continuously with the clock:

| | Night | Day |
|---|---|---|
| Ambient Ia | (0.11, 0.12, 0.20) bluish | (0.30, 0.30, 0.33) |
| Sky light | moon, blue-white, intensity ≤ 0.45 | sun, warm white → orange near sunset, ≤ 0.9 |
| Lamp | on (automatic), flickering | off (automatic) |
| Ghost glow | on | off |

`daylight = smoothstep(−0.1, 0.25, sin(sunAngle))` blends between the two smoothly at dawn and dusk.

## 5. Materials (examples)

| Material | colour | ks | n | special |
|---|---|---|---|---|
| floor | wood texture | 0.35 | 48 | reflectivity 0.18 |
| wallpaper | texture | 0.05 | 8 | nearly matte |
| boots | brown | 0.6 | 48 | shiny leather |
| eye-pupil | near black | 0.9 | 128 | tiny sharp highlight (a "living" eye) |
| car-paint | red | 0.9 | 96 | glossy, reflectivity 0.15 |
| Buzz-helmet | pale blue | 1.0 | 128 | opacity 0.22, reflectivity 0.25 |
| bulb / sun / moon / sky | — | — | — | `unlit`, emissive |

## 6. Live demonstration (keys)

| Key | Effect |
|---|---|
| F5 | ambient term on/off |
| F6 | diffuse term on/off |
| F7 | specular term on/off |
| F8 | sun/moon light on/off |
| 7 then R | lamp power; `,` / `.` brightness; W/S tilt the cone; A/D swivel |
| 5 then L | car headlights |
| 4 then L | Buzz's laser (red light on nearby surfaces) |
| `,` `.` `[` `]` P | move / speed up / pause the clock to see dawn, day, dusk, night |

Turning off diffuse and specular leaves the flat ambient silhouette; turning on only specular shows the
highlights alone — a direct visual decomposition of the equation above.

## 7. Limits of the raster path (and why ray tracing exists)

The rasteriser evaluates the formula for each fragment **independently**: it does not know whether
something stands between P and the light. So in raster mode sunlight also reaches the floor "through"
the walls, and there are no shadows. The ray-traced mode ([12](12-ray-tracing.md)) adds a visibility term
per light (shadow rays), plus reflections and transparency.
