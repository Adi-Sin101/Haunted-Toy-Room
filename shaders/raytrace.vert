#version 330 core
// Full-screen triangle generated from gl_VertexID (no vertex buffer needed):
//   id 0 -> (-1,-1)   id 1 -> (3,-1)   id 2 -> (-1,3)
// The triangle covers the whole screen; the part outside [-1,1] is clipped away.

out vec2 vNdc;

void main()
{
	vec2 p = vec2((gl_VertexID == 1) ? 3.0 : -1.0, (gl_VertexID == 2) ? 3.0 : -1.0);
	vNdc = p;
	gl_Position = vec4(p, 0.0, 1.0);
}
