#include "Texture.h"

#include <utility>

Texture::Texture(const std::string& name, const Image& image, bool nearest, bool repeat)
	: name(name), width(image.width), height(image.height)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);

	// Rows are tightly packed RGBA8, so the default 4-byte row alignment is always satisfied.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
	if (id != 0)
		glDeleteTextures(1, &id);
}

Texture::Texture(Texture&& other) noexcept
	: name(std::move(other.name)), id(std::exchange(other.id, 0)), width(other.width), height(other.height) {}

Texture& Texture::operator=(Texture&& other) noexcept
{
	if (this != &other) {
		if (id != 0)
			glDeleteTextures(1, &id);
		name = std::move(other.name);
		id = std::exchange(other.id, 0);
		width = other.width;
		height = other.height;
	}
	return *this;
}

void Texture::Bind(GLuint unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, id);
}
