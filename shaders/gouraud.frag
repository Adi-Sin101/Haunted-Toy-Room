#version 330 core
// GOURAUD shading, fragment stage: only combines the interpolated vertex lighting with the
// material colour and texture. No lighting maths happens per pixel.

#include "lighting.glsl"

in vec3 vLight;
in vec3 vSpecular;
in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uCameraPos;
uniform sampler2D uTexture;
uniform int uUseTexture;
uniform float uHighlight;

out vec4 FragColor;

void main()
{
	vec3 texColor = uUseTexture == 1 ? texture(uTexture, vUV * uMaterial.uvScale).rgb : vec3(1.0);
	vec3 albedo = uMaterial.color * texColor;

	if (uMaterial.unlit == 1) {
		FragColor = vec4(uMaterial.emissive + albedo, uMaterial.opacity);
		return;
	}
	if (uLightingEnabled == 0 || uShadingEnabled == 0) {
		vec3 illumination = uLightingEnabled == 1 ? basicIllumination(vWorldPos, uMaterial.ka, uMaterial.kd) : vec3(1.0);
		FragColor = vec4(albedo * illumination + uMaterial.emissive, uMaterial.opacity);
		return;
	}

	vec3 color = albedo * vLight + vSpecular + uMaterial.emissive;

	vec3 N = normalize(vNormal);
	vec3 V = normalize(uCameraPos - vWorldPos);
	float rim = pow(1.0 - abs(dot(N, V)), 2.0);
	color += uHighlight * (0.12 + 0.6 * rim) * vec3(1.0, 0.8, 0.2);

	FragColor = vec4(color, uMaterial.opacity);
}
