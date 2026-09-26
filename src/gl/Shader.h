#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

// Reads a whole text file. Throws std::runtime_error when the file cannot be opened.
std::string get_file_contents(const std::string& filename);

// A linked GLSL program (vertex + fragment shader).
// Supports `#include "file.glsl"` lines so lighting code is written once and shared by several shaders.
class Shader {
public:
	GLuint ID = 0;

	Shader() = default;
	Shader(const std::string& vertexFile, const std::string& fragmentFile);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;

	void Activate() const;
	void Delete();

	// Uniform locations are looked up once and cached; unknown names return -1 (ignored by GL).
	GLint Uniform(const std::string& name) const;

	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetVec2(const std::string& name, const glm::vec2& value) const;
	void SetVec3(const std::string& name, const glm::vec3& value) const;
	void SetVec4(const std::string& name, const glm::vec4& value) const;
	void SetMat3(const std::string& name, const glm::mat3& value) const;
	void SetMat4(const std::string& name, const glm::mat4& value) const;

private:
	mutable std::unordered_map<std::string, GLint> uniformCache;

	// Checks if the different Shaders have compiled / linked properly and prints the log if not.
	static void compileErrors(GLuint shader, bool isProgram, const std::string& label);
};

#endif
