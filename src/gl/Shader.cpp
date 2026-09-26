#include "Shader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glm/gtc/type_ptr.hpp>

#include "core/Paths.h"

std::string get_file_contents(const std::string& filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (!in)
		throw std::runtime_error("Cannot open file: " + filename);
	std::ostringstream contents;
	contents << in.rdbuf();
	return contents.str();
}

namespace {

// Replaces every line `#include "name"` with the contents of that file (relative to the including file).
std::string preprocess(const std::filesystem::path& file, int depth = 0)
{
	if (depth > 8)
		throw std::runtime_error("Shader #include nested too deeply: " + file.string());

	std::istringstream source(get_file_contents(file.string()));
	std::string out, line;
	while (std::getline(source, line)) {
		const auto pos = line.find("#include");
		if (pos != std::string::npos) {
			const auto first = line.find('"', pos);
			const auto last = line.find('"', first + 1);
			if (first != std::string::npos && last != std::string::npos) {
				out += preprocess(file.parent_path() / line.substr(first + 1, last - first - 1), depth + 1);
				out += '\n';
				continue;
			}
		}
		out += line;
		out += '\n';
	}
	return out;
}

GLuint compileStage(GLenum stage, const std::string& path)
{
	const std::string code = preprocess(Paths::resolve(path));
	const char* source = code.c_str();

	GLuint shader = glCreateShader(stage);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	return shader;
}

} // namespace

Shader::Shader(const std::string& vertexFile, const std::string& fragmentFile)
{
	GLuint vertexShader = compileStage(GL_VERTEX_SHADER, vertexFile);
	compileErrors(vertexShader, false, vertexFile);

	GLuint fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentFile);
	compileErrors(fragmentShader, false, fragmentFile);

	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);
	compileErrors(ID, true, vertexFile + " + " + fragmentFile);

	// The program keeps the compiled code; the individual shader objects are no longer needed.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
	Delete();
}

Shader::Shader(Shader&& other) noexcept
	: ID(std::exchange(other.ID, 0)), uniformCache(std::move(other.uniformCache)) {}

Shader& Shader::operator=(Shader&& other) noexcept
{
	if (this != &other) {
		Delete();
		ID = std::exchange(other.ID, 0);
		uniformCache = std::move(other.uniformCache);
	}
	return *this;
}

void Shader::Activate() const
{
	glUseProgram(ID);
}

void Shader::Delete()
{
	if (ID != 0) {
		glDeleteProgram(ID);
		ID = 0;
	}
	uniformCache.clear();
}

GLint Shader::Uniform(const std::string& name) const
{
	auto it = uniformCache.find(name);
	if (it != uniformCache.end())
		return it->second;
	GLint location = glGetUniformLocation(ID, name.c_str());
	uniformCache.emplace(name, location);
	return location;
}

void Shader::SetInt(const std::string& name, int value) const { glUniform1i(Uniform(name), value); }
void Shader::SetFloat(const std::string& name, float value) const { glUniform1f(Uniform(name), value); }
void Shader::SetVec2(const std::string& name, const glm::vec2& value) const { glUniform2fv(Uniform(name), 1, glm::value_ptr(value)); }
void Shader::SetVec3(const std::string& name, const glm::vec3& value) const { glUniform3fv(Uniform(name), 1, glm::value_ptr(value)); }
void Shader::SetVec4(const std::string& name, const glm::vec4& value) const { glUniform4fv(Uniform(name), 1, glm::value_ptr(value)); }
void Shader::SetMat3(const std::string& name, const glm::mat3& value) const { glUniformMatrix3fv(Uniform(name), 1, GL_FALSE, glm::value_ptr(value)); }
void Shader::SetMat4(const std::string& name, const glm::mat4& value) const { glUniformMatrix4fv(Uniform(name), 1, GL_FALSE, glm::value_ptr(value)); }

void Shader::compileErrors(GLuint shader, bool isProgram, const std::string& label)
{
	GLint ok = GL_FALSE;
	char infoLog[2048];
	if (!isProgram) {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (ok == GL_FALSE) {
			glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
			std::cerr << "SHADER_COMPILATION_ERROR in " << label << "\n" << infoLog << std::endl;
			throw std::runtime_error("Shader compilation failed: " + label);
		}
	}
	else {
		glGetProgramiv(shader, GL_LINK_STATUS, &ok);
		if (ok == GL_FALSE) {
			glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
			std::cerr << "SHADER_LINKING_ERROR for " << label << "\n" << infoLog << std::endl;
			throw std::runtime_error("Shader linking failed: " + label);
		}
	}
}
