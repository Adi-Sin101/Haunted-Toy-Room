#version 330 core
// Draws the low-resolution ray-traced image over the whole window (bilinear upscale).
// (glBlitFramebuffer cannot be used: the window's framebuffer is multisampled.)

in vec2 vNdc;
out vec4 FragColor;

uniform sampler2D uImage;

void main()
{
	FragColor = texture(uImage, vNdc * 0.5 + 0.5);
}
