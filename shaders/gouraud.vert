#version 330 core
// GOURAUD shading (Lecture 9): the illumination model is evaluated once PER VERTEX here, and the
// resulting colours are linearly interpolated across the triangle by the rasteriser.
// Cheap, but highlights that fall between vertices are missed or smeared.

#include "lighting.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix;
uniform vec3 uCameraPos;

out vec3 vLight;     // ambient + diffuse light arriving at the vertex
out vec3 vSpecular;  // specular highlight at the vertex
out vec2 vUV;
out vec3 vNormal;
out vec3 vWorldPos;

void main()
{
	vec4 world = uModel * vec4(aPos, 1.0);
	vec3 N = normalize(uNormalMatrix * aNormal);
	vec3 V = normalize(uCameraPos - world.xyz);

	vec3 diffuse, specular;
	computeLighting(world.xyz, N, V, diffuse, specular);
	vLight = ambientTerm() + diffuse;
	vSpecular = specular;
	vUV = aUV;
	vNormal = N;
	vWorldPos = world.xyz;

	gl_Position = uProj * uView * world;
}
