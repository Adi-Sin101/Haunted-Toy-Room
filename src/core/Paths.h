#pragma once

#include <filesystem>
#include <string>

// Finds run-time resources (shaders/, assets/) regardless of the working directory:
// looks next to the executable first, then walks up the parent folders (so running from
// the project root or from bin/Debug both work).
namespace Paths {

void init(const char* argv0);
std::filesystem::path resolve(const std::string& relative);

}
