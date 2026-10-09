#version 330 core
// Raster sky: the view direction through this pixel is rebuilt with the inverse of the camera's
// rotation-only view-projection, then shaded with the same skyRadiance() the ray tracer uses.

#include "sky.glsl"

in vec2 vNdc;
out vec4 FragColor;

uniform mat4 uInvViewProj; // inverse(proj * view-without-translation)

void main()
{
	vec4 far = uInvViewProj * vec4(vNdc, 1.0, 1.0);
	FragColor = vec4(skyRadiance(far.xyz / far.w), 1.0);
}
