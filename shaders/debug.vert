#version 330 core
// Debug overlay: coloured lines (normals, axes gizmo) and vertex points.

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform int uUseVertexColor;
uniform vec3 uColor;
uniform float uPointSize;

out vec3 vColor;

void main()
{
	vColor = uUseVertexColor == 1 ? aColor : uColor;
	gl_PointSize = uPointSize;
	gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
}
