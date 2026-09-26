// ============================================================================================
// lighting.glsl - shared illumination code (Lecture 8), #included by the raster shaders and
// the ray tracer so every render path uses exactly the same light model.
//
//   I = ka*Ia*C  +  sum_i  att_i * spot_i * Il_i * ( kd*max(N.L,0)*C  +  ks*spec_i )  +  E
//
//   Phong  specular: spec = max(R.V, 0)^n         R = reflect(-L, N)
//   Blinn  specular: spec = max(N.H, 0)^(4n)      H = normalize(L + V)   (4n keeps a similar size)
// ============================================================================================

#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2

struct Light {
	int type;
	int enabled;
	vec3 position;
	vec3 direction;   // direction the light travels
	vec3 color;
	float intensity;
	float constant;   // attenuation 1 / (kc + kl*d + kq*d^2)
	float linear;
	float quadratic;
	float innerCutoff; // cos of inner half-angle
	float outerCutoff; // cos of outer half-angle
};

struct Material {
	vec3 color;
	float ka;
	float kd;
	float ks;
	float shininess;
	vec3 emissive;
	float opacity;
	int unlit;
	vec2 uvScale;
};

uniform Light uLights[MAX_LIGHTS];
uniform int uLightCount;
uniform vec3 uAmbientLight;   // Ia
uniform Material uMaterial;
uniform int uUseAmbient;
uniform int uUseDiffuse;
uniform int uUseSpecular;
uniform int uBlinn;           // 1 = Blinn-Phong half vector, 0 = Phong reflection vector

// Direction TO the light (L), distance-based attenuation and spot-cone factor for light i at point P.
void lightVector(int i, vec3 P, out vec3 L, out float attenuation, out float distanceToLight)
{
	Light light = uLights[i];
	if (light.type == LIGHT_DIRECTIONAL) {
		L = normalize(-light.direction);
		attenuation = 1.0;
		distanceToLight = 1e6;
		return;
	}
	vec3 toLight = light.position - P;
	distanceToLight = length(toLight);
	L = toLight / max(distanceToLight, 1e-4);
	attenuation = 1.0 / (light.constant + light.linear * distanceToLight + light.quadratic * distanceToLight * distanceToLight);
	if (light.type == LIGHT_SPOT) {
		// cos of the angle between the spot axis and the ray from the light to P
		float theta = dot(-L, normalize(light.direction));
		attenuation *= smoothstep(light.outerCutoff, light.innerCutoff, theta);
	}
}

// Adds light i's diffuse and specular contribution (without the object colour) at point P.
// `visibility` is 1 when lit, 0 in shadow (the raster path always passes 1).
// kd, ks, shininess are passed explicitly so the ray tracer can use per-hit materials.
void addLight(int i, vec3 P, vec3 N, vec3 V, float visibility, float kd, float ks, float shininess,
              inout vec3 diffuse, inout vec3 specular)
{
	vec3 L; float att; float dist;
	lightVector(i, P, L, att, dist);
	vec3 radiance = uLights[i].color * uLights[i].intensity * att * visibility;

	float NdotL = max(dot(N, L), 0.0);
	if (uUseDiffuse == 1)
		diffuse += kd * NdotL * radiance;

	if (uUseSpecular == 1 && NdotL > 0.0) {
		float s;
		if (uBlinn == 1) {
			vec3 H = normalize(L + V);
			s = pow(max(dot(N, H), 0.0), shininess * 4.0);
		} else {
			vec3 R = reflect(-L, N);
			s = pow(max(dot(R, V), 0.0), shininess);
		}
		specular += ks * s * radiance;
	}
}

vec3 ambientTerm(float ka)
{
	return uUseAmbient == 1 ? ka * uAmbientLight : vec3(0.0);
}

vec3 ambientTerm()
{
	return ambientTerm(uMaterial.ka);
}

// Diffuse + specular of all enabled lights (no shadows).
void computeLighting(vec3 P, vec3 N, vec3 V, out vec3 diffuse, out vec3 specular)
{
	diffuse = vec3(0.0);
	specular = vec3(0.0);
	for (int i = 0; i < uLightCount; ++i) {
		if (uLights[i].enabled == 1)
			addLight(i, P, N, V, 1.0, uMaterial.kd, uMaterial.ks, uMaterial.shininess, diffuse, specular);
	}
}
