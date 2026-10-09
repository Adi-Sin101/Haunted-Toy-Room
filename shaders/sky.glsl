// ============================================================================================
// sky.glsl - the procedural sky dome, #included by the raster sky pass (sky.frag) and by the ray
// tracer (a ray that hits nothing returns skyRadiance(direction)), so both paths show the same sky.
//
// The sky is a function of DIRECTION only (it is infinitely far away):
//   gradient   horizon colour -> zenith colour, by elevation (night navy, day blue, sunset orange)
//   sun        a sharp disc plus two glows (Mie-like forward scattering around the sun)
//   moon       a disc with darker maria from value noise, limb darkening and a pale halo
//   stars      one candidate star per cell of a 3D grid over the unit sphere, twinkling, night only
//   clouds     fractal value noise on a cloud plane above the camera, lit by the sun or the moon
// The sun and moon directions are the SAME vectors the directional light uses outdoors, so the
// sunlight and the shadows on the garden always come from the visible sun or moon.
// ============================================================================================

uniform vec3 uSunDir;        // unit vector TOWARD the sun
uniform vec3 uMoonDir;       // unit vector TOWARD the moon
uniform vec3 uSkyZenith;     // sky colour straight up
uniform vec3 uSkyHorizon;    // sky colour at the horizon (also the fog colour)
uniform vec3 uSunColor;
uniform float uDaylight;     // 0 night .. 1 day
uniform float uSunsetGlow;   // 0 .. 1, strongest while the sun is near the horizon
uniform float uStarAmount;   // 0 .. 1, stars fade out at dawn
uniform float uSkyTime;      // seconds, drifts the clouds and twinkles the stars

float skyHash(vec3 p)
{
	p = fract(p * vec3(0.1031, 0.1030, 0.0973));
	p += dot(p, p.yxz + 33.33);
	return fract((p.x + p.y) * p.z);
}

float skyNoise(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	float a = skyHash(vec3(i, 1.0)), b = skyHash(vec3(i + vec2(1, 0), 1.0));
	float c = skyHash(vec3(i + vec2(0, 1), 1.0)), d = skyHash(vec3(i + vec2(1, 1), 1.0));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float skyFbm(vec2 p)
{
	float sum = 0.0, amplitude = 0.5;
	for (int i = 0; i < 4; ++i) { sum += amplitude * skyNoise(p); p = p * 2.03 + vec2(17.1, 9.2); amplitude *= 0.5; }
	return sum;
}

vec3 skyStars(vec3 dir)
{
	if (uStarAmount <= 0.001 || dir.y <= 0.0) return vec3(0.0);
	vec3 p = dir * 220.0;
	vec3 cell = floor(p);
	float h = skyHash(cell);
	if (h < 0.9965) return vec3(0.0);
	vec3 star = cell + vec3(skyHash(cell + 7.1), skyHash(cell + 3.7), skyHash(cell + 1.3));
	float d = length(p - star);
	float twinkle = 0.65 + 0.35 * sin(uSkyTime * (2.0 + 3.0 * h) + h * 400.0);
	float size = smoothstep(0.55, 0.0, d);
	vec3 tint = mix(vec3(0.75, 0.82, 1.0), vec3(1.0, 0.92, 0.78), skyHash(cell + 11.0));
	return tint * size * twinkle * (h - 0.9965) / 0.0035 * 1.6 * uStarAmount * smoothstep(0.0, 0.25, dir.y);
}

vec3 skyMoon(vec3 dir, out float coverage)
{
	coverage = 0.0;
	float c = dot(dir, uMoonDir);
	const float radius = 0.030;            // about 1.7 degrees: larger than life, readable on screen
	if (c < cos(radius * 6.0)) return vec3(0.0);
	// Local 2D coordinates on the disc
	vec3 right = normalize(cross(abs(uMoonDir.y) > 0.95 ? vec3(1, 0, 0) : vec3(0, 1, 0), uMoonDir));
	vec3 up = cross(uMoonDir, right);
	vec2 q = vec2(dot(dir, right), dot(dir, up)) / radius;
	float r = length(q);
	float halo = pow(max(c, 0.0), 2200.0) * 0.35 + pow(max(c, 0.0), 300.0) * 0.08;
	vec3 glow = vec3(0.55, 0.65, 0.9) * halo * (1.0 - uDaylight * 0.8);
	if (r > 1.0) return glow;
	coverage = smoothstep(1.0, 0.92, r);
	float maria = skyFbm(q * 2.3 + 4.0);
	float limb = sqrt(max(1.0 - r * r, 0.0));
	vec3 surface = vec3(0.92, 0.93, 0.96) * mix(0.62, 1.0, smoothstep(0.35, 0.62, maria)) * (0.55 + 0.45 * limb);
	return glow + surface * coverage * mix(1.0, 0.45, uDaylight);
}

vec3 skyClouds(vec3 dir, vec3 background, out float density)
{
	density = 0.0;
	if (dir.y <= 0.01) return background;
	// Project onto a plane high above the camera: horizon clouds are compressed and far away.
	vec2 uv = dir.xz / (dir.y + 0.08) * 1.4 + vec2(uSkyTime * 0.012, uSkyTime * 0.004);
	float n = skyFbm(uv);
	density = smoothstep(0.50, 0.78, n) * smoothstep(0.01, 0.22, dir.y);
	if (density <= 0.0) return background;
	// Lit from the sun by day, from the moon (faintly, blue) at night; edges catch the sunset.
	vec3 dayLit = mix(vec3(0.96, 0.96, 0.98), uSunColor * 1.05, uSunsetGlow * 0.7);
	vec3 nightLit = vec3(0.10, 0.12, 0.18) + vec3(0.10, 0.12, 0.18) * max(dot(dir, uMoonDir), 0.0);
	vec3 lit = mix(nightLit, dayLit, uDaylight);
	float shade = mix(0.72, 1.0, smoothstep(0.55, 0.95, n));
	return mix(background, lit * shade, density * mix(0.55, 0.85, uDaylight));
}

vec3 skyRadiance(vec3 dir)
{
	dir = normalize(dir);
	float up = dir.y;
	// Below the horizon: the ground haze, so the edge of the lawn melts into the horizon.
	if (up < 0.0) return uSkyHorizon * mix(1.0, 0.75, smoothstep(0.0, -0.3, up));

	float elevation = pow(up, 0.45);
	vec3 color = mix(uSkyHorizon, uSkyZenith, elevation);

	// Sunset: an orange band on the horizon, brightest toward the sun's azimuth.
	vec2 sunFlat = normalize(uSunDir.xz + vec2(1e-4));
	vec2 dirFlat = normalize(dir.xz + vec2(1e-4));
	float towardSun = 0.5 + 0.5 * dot(sunFlat, dirFlat);
	color += vec3(0.85, 0.35, 0.10) * uSunsetGlow * pow(1.0 - up, 6.0) * mix(0.35, 1.0, towardSun);

	color += skyStars(dir) * (1.0 - uDaylight);

	float moonCoverage;
	vec3 moon = skyMoon(dir, moonCoverage);
	float cloudDensity;
	color = color * (1.0 - moonCoverage) + moon;
	color = skyClouds(dir, color, cloudDensity);

	// Sun: disc and glows (the glows also brighten the clouds near the sun).
	float s = max(dot(dir, uSunDir), 0.0);
	float sunVisible = smoothstep(-0.10, 0.02, uSunDir.y);
	float disc = smoothstep(0.99955, 0.99975, s) * (1.0 - cloudDensity * 0.8);
	color += uSunColor * (pow(s, 900.0) * 0.6 + pow(s, 24.0) * 0.18 * (0.4 + uSunsetGlow)) * sunVisible;
	color += vec3(1.0, 0.95, 0.82) * disc * 4.0 * sunVisible;
	return color;
}
