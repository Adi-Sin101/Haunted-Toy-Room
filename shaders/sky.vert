#version 330 core
// Sky pass: a full-screen triangle (from gl_VertexID, like raytrace.vert) placed ON the far plane
// (z = w gives depth 1.0). Drawn after the opaque scene with depth test LEQUAL, it only shades the
// pixels nothing else covered, so the sky costs nothing behind walls.

out vec2 vNdc;

void main()
{
	vec2 p = vec2((gl_VertexID == 1) ? 3.0 : -1.0, (gl_VertexID == 2) ? 3.0 : -1.0);
	vNdc = p;
	gl_Position = vec4(p, 1.0, 1.0);
}
