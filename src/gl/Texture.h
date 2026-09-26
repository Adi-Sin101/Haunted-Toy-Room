#pragma once

#include <string>

#include <glad/glad.h>

#include "render/Image.h"

// A 2D OpenGL texture created from an Image.
//
// Sampling settings:
//   wrap      GL_REPEAT so uv > 1 tiles the image (floor planks, wallpaper)
//   min filter GL_LINEAR_MIPMAP_LINEAR (trilinear) - distant surfaces sample pre-shrunk mipmaps,
//             which removes shimmering/aliasing
//   mag filter GL_LINEAR (or GL_NEAREST for crisp pixel-art look)
class Texture {
public:
	Texture(const std::string& name, const Image& image, bool nearest = false, bool repeat = true);
	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	void Bind(GLuint unit) const;

	GLuint ID() const { return id; }
	const std::string& Name() const { return name; }
	int Width() const { return width; }
	int Height() const { return height; }

private:
	std::string name;
	GLuint id = 0;
	int width = 0;
	int height = 0;
};
