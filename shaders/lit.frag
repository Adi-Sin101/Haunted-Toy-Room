#version 330 core
// Fragment shader for per-pixel shading (Lecture 9).
//   uShadingMode 0 = FLAT  : one normal per triangle, rebuilt from screen-space derivatives of the
//                            position (dFdx x dFdy lies in the triangle's plane) -> faceted look
//   uShadingMode 2 = PHONG : interpolated vertex normal, renormalised, lighting per fragment
//   (Gouraud uses gouraud.vert/frag; Blinn vs Phong specular is selected by uBlinn)

#include "lighting.glsl"
#include "fog.glsl"

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3 uCameraPos;
uniform int uShadingMode;
uniform sampler2D uTexture;
uniform int uUseTexture;
uniform float uHighlight;
uniform int uCutout;       // 1: texture alpha < 0.5 is a hole (fence, railing)  // > 0 for the selected object

out vec4 FragColor;

void main()
{
	vec4 texel = texture(uTexture, vUV * uMaterial.uvScale);
	if (uCutout == 1 && texel.a < 0.5)
		discard;
	vec3 texColor = uUseTexture == 1 ? texel.rgb : vec3(1.0);
	vec3 albedo = uMaterial.color * texColor;

	if (uMaterial.unlit == 1) {
		FragColor = vec4(applyFog(uMaterial.emissive + albedo,length(vWorldPos-uCameraPos)), uMaterial.opacity);
		return;
	}
	if (uLightingEnabled == 0 || uShadingEnabled == 0) {
		vec3 illumination = uLightingEnabled == 1 ? basicIllumination(vWorldPos, uMaterial.ka, uMaterial.kd) : vec3(1.0);
		FragColor = vec4(applyFog(albedo * illumination + uMaterial.emissive,length(vWorldPos-uCameraPos)), uMaterial.opacity);
		return;
	}

	vec3 N = uShadingMode == 0 ? normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)))
	                           : normalize(vNormal);
	vec3 V = normalize(uCameraPos - vWorldPos);
	// Light the side of a thin surface that faces the camera. (The flat normal from derivatives
	// already faces the camera, so it is never flipped.)
	if (!gl_FrontFacing && uShadingMode != 0)
		N = -N;

	vec3 diffuse, specular;
	computeLighting(vWorldPos, N, V, diffuse, specular);

	vec3 color = albedo * (ambientTerm() + diffuse) + specular + uMaterial.emissive;

	// Selection highlight: pulsing rim light on silhouettes.
	float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
	color += uHighlight * (0.12 + 0.6 * rim) * vec3(1.0, 0.8, 0.2);

	FragColor = vec4(applyFog(color,length(vWorldPos-uCameraPos)), uMaterial.opacity);
}
