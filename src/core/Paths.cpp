#include "Paths.h"

#include <vector>

namespace {
std::vector<std::filesystem::path> searchRoots;
}

namespace Paths {

void init(const char* argv0)
{
	namespace fs = std::filesystem;
	searchRoots.clear();

	std::error_code ec;
	fs::path exeDir = fs::absolute(fs::path(argv0), ec).parent_path();
	for (fs::path dir = exeDir; !dir.empty(); dir = dir.parent_path()) {
		searchRoots.push_back(dir);
		if (dir == dir.parent_path())
			break;
	}
	searchRoots.push_back(fs::current_path(ec));
}

std::filesystem::path resolve(const std::string& relative)
{
	namespace fs = std::filesystem;
	for (const auto& root : searchRoots) {
		fs::path candidate = root / relative;
		std::error_code ec;
		if (fs::exists(candidate, ec))
			return candidate;
	}
	return relative; // let the caller report "cannot open"
}

}
