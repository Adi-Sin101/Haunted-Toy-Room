#version 330 core
// Vertex shader for Flat / Phong / Blinn-Phong shading: transforms the vertex and passes the
// world-space position, normal and uv to the fragment shader, where lighting is evaluated per pixel.

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix; // transpose(inverse(mat3(uModel)))

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
	vec4 world = uModel * vec4(aPos, 1.0);
	vWorldPos = world.xyz;
	vNormal = uNormalMatrix * aNormal;
	vUV = aUV;
	gl_Position = uProj * uView * world; // clip space: projection * view * model * vertex
}
