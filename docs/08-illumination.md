# 08 — Illumination (Lecture 8)

Files: `shaders/lighting.glsl` (the model, shared by every shader and the ray tracer), `shaders/lit.frag`,
`shaders/gouraud.vert`, `src/scene/Light.h`, `src/scene/Material.h`, `Renderer::UploadLights`,
`Renderer::RenderLampShadow`, `ToyRoomApp::UpdateLights`, `src/world/Environment.cpp`.

*Illumination* answers: **how much light leaves a surface point P towards the eye?** *Shading*
([09](09-shading.md)) answers: **at which points do we evaluate that?** This chapter derives the formula
term by term, lists every light and constant in the scene, and ends with a worked numerical example.

## 1. The vectors at a surface point

Everything is computed in **world space** from four unit vectors at the point P:

```
N  surface normal                 N = normalize( NormalMatrix · n_object )        (see §8)
L  direction from P to the light  L = normalize( light.position − P )             (point / spot)
                                  L = normalize( −light.direction )               (directional)
V  direction from P to the eye    V = normalize( cameraPosition − P )
R  mirror reflection of L about N R = 2 (N·L) N − L                              (GLSL: reflect(−L, N))
H  half vector between L and V    H = normalize( L + V )                          (Blinn)
```

*Derivation of R.* Split L into a part along N, `(N·L)N`, and a part in the surface plane,
`L − (N·L)N`. A mirror keeps the normal part and flips the in-plane part:
`R = (N·L)N − (L − (N·L)N) = 2(N·L)N − L`.

## 2. The Phong illumination model

For a material with colour C (diffuse reflectance × texture), coefficients ka, kd, ks, shininess n and
emission E, with ambient light Ia and lights i = 1…k of radiance `Il_i`:

```
I = ka · Ia · C                                                         ambient
  + Σ_i  att_i · spot_i · vis_i · Il_i · [ kd · C · max(N·L_i, 0)        diffuse  (Lambert)
                                         + ks · spec_i ]                  specular
  + E                                                                   emission

spec_i = max(R_i·V, 0)^n                (Phong)      or      max(N·H_i, 0)^(4n)      (Blinn-Phong)
```

| Symbol | Meaning | In code |
|---|---|---|
| C | object colour × texture colour (`albedo`) | `uMaterial.color * texture(uTexture, uv·uvScale)` |
| ka, kd, ks | ambient / diffuse / specular reflection coefficients | `Material::ka, kd, ks` (default 1, 1, 0.3) |
| n | specular exponent (shininess) | `Material::shininess` |
| Ia | ambient light colour | `uAmbientLight` ← `Environment::AmbientLight()` |
| Il_i | light colour × intensity | `uLights[i].color * uLights[i].intensity` |
| att_i | distance attenuation (§3) | `lightVector()` |
| spot_i | spot-cone factor (§3) | `lightVector()` |
| vis_i | visibility: 1 lit, 0 in shadow (§6) | `lampVisibility()` / ray-traced shadow rays |
| E | emission (bulb, laser, sky, sun, moon, headlights, porch lantern) | `Material::emissive` |

The shader computes the **light** first and multiplies by the colour at the end:

```glsl
vec3 color = albedo * (ambientTerm() + diffuse) + specular + uMaterial.emissive;
```

So diffuse light is tinted by the surface colour while **specular highlights keep the light's colour**,
which is what a plastic toy does (the highlight on a red car is white, not red).

### 2.1 Ambient
A constant approximation of light that has bounced around the room: `ka·Ia·C`. It keeps faces that no
light reaches from being pure black. It does not depend on N, L or V.

### 2.2 Diffuse — Lambert's cosine law
A beam of cross-section A hitting a surface at angle θ to the normal spreads over an area `A / cos θ`,
so the energy per unit area falls with `cos θ = N·L`. A matte surface scatters that energy equally in all
directions, so the result does not depend on V:

```
diffuse = kd · C · Il · max(N·L, 0)
```

`max(…, 0)` removes light arriving from behind the surface.

### 2.3 Specular — Phong and Blinn-Phong
A shiny surface reflects most strongly around the mirror direction R. Phong models the falloff as a
power of the cosine between R and V: `max(R·V, 0)^n`. Large n → small sharp highlight (eye pupils n = 128);
small n → broad dull sheen (walls n = 8).

Blinn's variant measures how far N is from the half vector H instead: N·H = 1 exactly when V is the
mirror direction of L. The angle between N and H is about half the angle between R and V, so the
exponent is multiplied by 4 to give a highlight of similar size. Blinn-Phong is cheaper (no reflect) and
behaves better at grazing angles; it is the default. **F2** switches models ([09](09-shading.md)).

The specular term is only added where `N·L > 0`, so the dark side of an object never shows a highlight.

**Skipping lights that cannot contribute** (`computeLighting`). For every light the shader first computes
L and the attenuation × spot factor; if `att·intensity < 10⁻⁴` (outside the spot cone, or too far away) or
`N·L ≤ 0` (behind the surface), the light is skipped before anything else — in particular before the
lamp's 9-sample shadow-map lookup (§6.2), which used to run for every fragment on screen even far outside
the lamp's cone. The image is unchanged (the skipped terms are ≤ 10⁻⁴); the per-pixel work drops.

## 3. Light types, attenuation and spot cones (`lightVector` in `lighting.glsl`)

| Type | L at P | att | Used for |
|---|---|---|---|
| Directional | `−direction` (same for every P) | 1 (infinitely far) | sun by day, moon by night |
| Point | `normalize(position − P)` | `1 / (kc + kl·d + kq·d²)` | lamp bulb, laser glow, ghost glow, hall night light |
| Spot | as point | as point × spot factor | lamp shade, RC car headlights |

**Attenuation.** Physically light falls with `1/d²`; the quadratic polynomial keeps a finite value at
d = 0 (`kc = 1`) and lets each light choose how quickly it fades.

**Spot cone.** With `cos θ = (−L)·axis` (θ = angle between the spot axis and the ray light→P) and the cone
given by the cosines of its inner and outer half-angles:

```
t    = clamp( (cos θ − cos outer) / (cos inner − cos outer), 0, 1 )
spot = t² (3 − 2t)                       GLSL smoothstep(cos outer, cos inner, cos θ)
```

spot = 1 inside the inner cone, 0 outside the outer cone, and an S-shaped soft edge in between.
Comparing cosines avoids any `acos` per pixel (cos is decreasing on [0°, 180°], so the inequality simply
flips).

## 4. The lights in the scene (`ToyRoomApp` light slots)

Positions and directions are copied from scene nodes every frame (`UpdateLights`), so lights follow the
hierarchy: tilt the lamp head and its cone moves; drive the car and the headlight beams sweep the room.

| Slot | Light | Type | Colour | kl / kq | Cone (inner / outer) | Intensity | Ray-traced shadows |
|---|---|---|---|---|---|---|---|
| 0 | Sun / Moon | directional | §5 | — | — | ≤ 0.9 / ≤ 0.8 | yes |
| 1 | Lamp bulb | point | (1.00, 0.82, 0.55) | 0.14 / 0.07 | — | 0.5 · lamp | yes |
| 2 | Lamp spot | spot | (1.00, 0.88, 0.65) | 0.05 / 0.01 | 22° / 34° | 1.6 · lamp | yes (+ raster shadow map) |
| 3 | Hall night light | point at (13.5, 3.7, 4) | (0.75, 0.80, 1.00) | 0.10 / 0.03 | — | 0.85 | no |
| 4, 5 | Car headlights | spot along the car's +Z, 0.15 down | (1.00, 0.95, 0.80) | 0.10 / 0.05 | 12° / 22° | 1.5 | no |
| 6 | Laser glow | point at Buzz's laser tip | (1.00, 0.10, 0.10) | 0.35 / 0.40 | — | 1.5 | no |
| 7 | Ghost glow | point at the ghost | (0.50, 0.70, 1.00) | 0.30 / 0.15 | — | 0.8 · ghost visibility | no |

`lamp = lampBrightness (1.6, keys , .) × flicker`, so the lamp spot shines with 2.56 and the bulb with
0.8 at full power. All lights have kc = 1. Up to `MaxLights = 8` lights are uploaded as the uniform array
`uLights[i]` plus `uLightCount`.

## 5. Day, night and the sun (`Environment`)

The clock angle `a = (hour − 6) / 12 · π` is 0 at 06:00, π/2 at noon and π at 18:00; `sunHeight = sin a`.

```
daylight     = smoothstep(−0.1, 0.25, sunHeight)                         0 night … 1 day
Ia           = mix( (0.18, 0.19, 0.26), (0.48, 0.46, 0.43), daylight )   bluish night → warm day
sky light    = sun   while sunHeight ≥ 0:  colour mix((1, 0.95, 0.85), (1, 0.55, 0.3), sunset),
                                           intensity 0.9 · smoothstep(−0.05, 0.3, sunHeight)
               moon  otherwise:            colour (0.55, 0.65, 1.0), intensity 0.8 · smoothstep(−0.05, 0.3, −sunHeight)
sunset       = 1 − smoothstep(0, 0.4, sunHeight)
direction    = normalize( roomCentre − position of the sun or moon sphere )
sky colour   = mix((0.02, 0.03, 0.10), (0.45, 0.65, 0.95), daylight) + (0.55, 0.22, 0.05)·exp(−6|sunHeight|)
```

The last term adds orange near the horizon at dawn and dusk. The sky plane is `unlit`, its emission is
the sky colour and its texture (the stars) is multiplied by `1 − daylight`, so stars fade out by day.

**The garden sun.** During Penny's arrival ([18](18-house-and-penny.md)) the sun is also shown from the
garden: `Environment::OutdoorSunPosition() = (−60 cos a, 4 + 55 sin a, −37)`, the same angle on a much
larger arc just in front of the sky backdrop. Walking from 16:18 to dusk, it visibly sinks towards the
horizon behind the house.

**Haunted lamp.** At night the lamp switches on and flickers:
`flicker = 0.8 + 0.2·noise(9t)`, dropping to 15 % whenever a slower noise exceeds 0.78.

## 6. Shadows

### 6.1 Ray-traced mode
Each light with `castsShadows` sends a shadow ray from `P + 0.002·N` towards the light; if any opaque, lit
object is hit before the light, `vis = 0` ([12](12-ray-tracing.md)).

### 6.2 Raster mode: the lamp's shadow map (`Renderer::RenderLampShadow`)
Rasterisation has no visibility information, so the lamp spot uses a **shadow map**, built in a first pass
every frame while the lamp is on (and lighting + shading are enabled):

1. **Render depth from the lamp.** A perspective camera at the lamp, looking along the spot axis, with
   field of view = 2 × outer cone angle (68°), near 0.08, far 36:
   `LightVP = Perspective(68°, 1, 0.08, 36) · LookAt(lampPos, lampPos + axis, up)`.
   Every opaque shape is drawn into a 2048 × 2048 depth texture (no colour). Shapes outside the lamp's
   frustum and shapes with a bounding radius under 0.035 are skipped ([17](17-performance.md)).
2. **Look up per shaded point** (`lampVisibility` in `lighting.glsl`):
   ```
   clip  = LightVP · (P, 1)
   coord = clip.xyz / clip.w · 0.5 + 0.5           NDC [−1,1] → texture [0,1]; coord.z = P's depth seen from the lamp
   bias  = max( 0.0009 · (1 − N·L), 0.00012 )      larger at grazing angles, where depth precision is worst
   vis   = (1/9) Σ_{x,y ∈ {−1,0,1}}  [ coord.z − bias ≤ depthMap(coord.xy + (x, y)/2048) ]
   ```
   If the depth stored in the map is nearer than P, something stands between P and the lamp → shadow.
   Averaging 3 × 3 neighbouring comparisons (**percentage-closer filtering**) gives a soft edge instead of
   jagged texels. Points outside the map are treated as lit.
3. Only the lamp spot (slot 2) is multiplied by `vis`; other lights are unshadowed in raster mode.

**Worked example (a point at the edge of a shadow).** Suppose P projects to `coord.z = 0.600` and N·L = 0.8,
so `bias = max(0.0009 · 0.2, 0.00012) = 0.00018`. The 3 × 3 neighbourhood of the depth map holds 0.65 in
six texels (nothing in front of P: 0.59982 ≤ 0.65 → lit) and 0.42 in three texels (a toy's arm is closer
to the lamp: 0.59982 > 0.42 → shadowed). `vis = 6/9 = 0.667`, so the lamp spot's diffuse and specular at P
are multiplied by 0.667 — a soft penumbra pixel rather than a hard step.

**Skipped before the lookup.** For a point 50° away from the lamp's axis, `cos θ = 0.643 < cos 34° = 0.829`,
so `spot = 0`, `att·spot·intensity = 0 < 10⁻⁴` and the light is skipped in `computeLighting` before
`lampVisibility` is called — none of the 9 texture reads happens. Only the pixels inside the lamp's
68° cone pay for the shadow lookup.

**Contact shadows.** Under each toy a flattened, translucent (opacity 0.2) dark sphere is drawn on the
floor in raster mode. It is a cheap "ambient occlusion" cue that grounds the toys when the directional
light casts no raster shadow.

## 7. Modes of the Settings panel

The renderer evaluates three different expressions depending on the toggles (backtick opens Settings):

| Lighting | Shading | Fragment colour |
|---|---|---|
| off | — | `C + E` (flat albedo; the default preview) |
| on | off | `C · (ka·Ia + Σ kd·Il_i·att_i·spot_i) + E` — light colour and distance only, no N·L, no specular, no shadows (`basicIllumination`) |
| on | on | the full model of §2 with the selected shading model, lamp shadow map and contact shadows |

`unlit` materials (sky, sun, moon, bulb, laser, distant skyline, porch lantern) always output `E + C`.

**Selection highlight.** The selected object gets an extra rim term
`highlight · (0.12 + 0.6·(1 − N·V)²) · (1, 0.8, 0.2)`, strongest on its silhouette where N ⟂ V.

**Transparency.** Glass, ghost, laser, contact shadows and the sun's halo use `opacity < 1` and are drawn
after all opaque objects, far to near, with blending
`colour = α·source + (1 − α)·destination` and depth writes off.

Values are not tone-mapped: the 8-bit framebuffer clamps each channel to [0, 1].

## 8. Normals in world space — the normal matrix

Normals cannot be transformed with the model matrix M when M scales non-uniformly: a sphere squashed
into an ellipsoid would get normals that are no longer perpendicular to its surface. A tangent t on the
surface transforms with M (`t' = M t`), and the normal must stay perpendicular: `n'ᵀ t' = 0`.
Choosing `n' = (M⁻¹)ᵀ n` gives `n'ᵀ t' = nᵀ M⁻¹ M t = nᵀ t = 0` ✓. Hence

```
NormalMatrix = transpose( inverse( upper-left 3×3 of M ) )          t3d::normalMatrix
```

computed once per shape per frame on the CPU and uploaded as `uNormalMatrix`. The vertex shader
transforms and the fragment shader re-normalises (interpolation shortens unit vectors).

## 9. Worked example: one floor point lit by the lamp spot

Scene values (only the lamp spot on, night ambient, Blinn-Phong):

```
lamp spot    position (−4, 3.5, −7.5), colour (1, 0.88, 0.65), intensity 2.56, kc 1, kl 0.05, kq 0.01, cone 22°/34°
             axis (0.4488, −0.7854, 0.4263)
floor point  P = (−3, 0, −5), N = (0, 1, 0), material: ka = kd = 1, ks = 0.35, n = 48
             albedo C = (0.60, 0.40, 0.24)  (a wood-texture texel × white material colour)
camera       (0, 5, 8.2)      ambient Ia = (0.18, 0.19, 0.26)
```

| Step | Computation | Result |
|---|---|---|
| distance | d = \|(−4, 3.5, −7.5) − (−3, 0, −5)\| = \|(−1, 3.5, −2.5)\| | 4.416 |
| L | (−1, 3.5, −2.5) / 4.416 | (−0.2265, 0.7926, −0.5661) |
| V | normalize((0, 5, 8.2) − P) = normalize(3, 5, 13.2) | (0.2079, 0.3465, 0.9147) |
| attenuation | 1 / (1 + 0.05·4.416 + 0.01·4.416²) = 1 / 1.4158 | 0.7063 |
| spot | cos θ = −L·axis = 0.9655 (θ = 15.1°) > cos 22° = 0.9272 → inside the inner cone | 1 |
| radiance | (1, 0.88, 0.65) · 2.56 · 0.7063 · 1 | (1.808, 1.591, 1.175) |
| N·L | y-component of L | 0.7926 |
| diffuse light | 1 · 0.7926 · radiance | (1.433, 1.261, 0.932) |
| H | normalize(L + V) | (−0.0156, 0.9561, 0.2926) |
| Blinn | (N·H)^(4·48) = 0.9561^192 | 0.00018 |
| Phong (for comparison) | R = 2(N·L)N − L = (0.2265, 0.7926, 0.5661); (R·V)^48 = 0.8396^48 | 0.00023 |
| specular light | 0.35 · 0.00018 · radiance | ≈ (0.0001, 0.0001, 0.0001) |
| **final** | C · (ka·Ia + diffuse) + specular = (0.60·1.613, 0.40·1.451, 0.24·1.192) | **(0.968, 0.581, 0.286)** |

The point is brightly lit (well inside the cone, 4.4 units away) but shows no highlight: the eye is far
from the mirror direction (R·V = 0.84 → angle 33°, and 0.84^48 ≈ 0).

**The highlight case.** Moving to the point where the floor mirrors the lamp into the camera,
P = (−2.353, 0, −1.035) (the intersection of the floor with the line from the camera to the lamp's mirror
image (−4, −3.5, −7.5)), gives d = 7.534, attenuation 0.514, radiance (1.317, 1.159, 0.856), N·L = 0.465 and
**N·H = R·V = 1**: both specular models give exactly 1, so the specular light is
`0.35 · radiance = (0.461, 0.406, 0.300)` and the final colour is **(0.936, 0.697, 0.457)** — a bright warm
highlight that is visible on the polished floor.

The same numbers are produced in every render path: the fragment shader (Phong / Blinn / Flat), the
vertex shader (Gouraud, at the vertices) and the ray tracer all call the same `lighting.glsl` functions.
