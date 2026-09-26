#include "BmpLoader.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

std::uint32_t readU32(const std::uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24); }
std::uint16_t readU16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
std::int32_t readI32(const std::uint8_t* p) { return static_cast<std::int32_t>(readU32(p)); }

void writeU32(std::vector<std::uint8_t>& out, std::uint32_t v)
{
	for (int i = 0; i < 4; ++i)
		out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}
void writeU16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
	out.push_back(static_cast<std::uint8_t>(v & 0xFF));
	out.push_back(static_cast<std::uint8_t>(v >> 8));
}

} // namespace

namespace Bmp {

std::optional<Image> Load(const std::string& path)
{
	std::ifstream in(path, std::ios::binary);
	if (!in) {
		std::cerr << "BMP: cannot open " << path << "\n";
		return std::nullopt;
	}
	std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (file.size() < 54 || file[0] != 'B' || file[1] != 'M') {
		std::cerr << "BMP: not a bitmap file " << path << "\n";
		return std::nullopt;
	}

	// BITMAPFILEHEADER
	const std::uint32_t dataOffset = readU32(&file[10]);
	// BITMAPINFOHEADER
	const std::int32_t width = readI32(&file[18]);
	const std::int32_t rawHeight = readI32(&file[22]);
	const std::uint16_t bitsPerPixel = readU16(&file[28]);
	const std::uint32_t compression = readU32(&file[30]);

	const bool topDown = rawHeight < 0;
	const int height = std::abs(rawHeight);
	if (width <= 0 || height == 0 || (bitsPerPixel != 24 && bitsPerPixel != 32) || (compression != 0 && compression != 3)) {
		std::cerr << "BMP: unsupported format (" << bitsPerPixel << " bpp, compression " << compression << ") " << path << "\n";
		return std::nullopt;
	}

	const int bytesPerPixel = bitsPerPixel / 8;
	const size_t rowSize = (static_cast<size_t>(width) * bytesPerPixel + 3) & ~static_cast<size_t>(3); // pad to 4 bytes
	if (dataOffset + rowSize * height > file.size()) {
		std::cerr << "BMP: file truncated " << path << "\n";
		return std::nullopt;
	}

	Image img(width, height);
	for (int row = 0; row < height; ++row) {
		// Our Image stores the bottom row first, exactly like a bottom-up BMP.
		const int srcRow = topDown ? (height - 1 - row) : row;
		const std::uint8_t* src = &file[dataOffset + rowSize * srcRow];
		for (int x = 0; x < width; ++x) {
			const std::uint8_t* px = src + static_cast<size_t>(x) * bytesPerPixel;
			const size_t dst = (static_cast<size_t>(row) * width + x) * 4;
			img.pixels[dst + 0] = px[2]; // R  (stored as B, G, R)
			img.pixels[dst + 1] = px[1]; // G
			img.pixels[dst + 2] = px[0]; // B
			img.pixels[dst + 3] = bytesPerPixel == 4 ? px[3] : 255;
		}
	}
	return img;
}

bool Save(const std::string& path, const Image& image)
{
	const size_t rowSize = (static_cast<size_t>(image.width) * 3 + 3) & ~static_cast<size_t>(3);
	const std::uint32_t dataSize = static_cast<std::uint32_t>(rowSize * image.height);

	std::vector<std::uint8_t> out;
	out.reserve(54 + dataSize);
	// BITMAPFILEHEADER
	out.push_back('B'); out.push_back('M');
	writeU32(out, 54 + dataSize);
	writeU32(out, 0);
	writeU32(out, 54);
	// BITMAPINFOHEADER
	writeU32(out, 40);
	writeU32(out, static_cast<std::uint32_t>(image.width));
	writeU32(out, static_cast<std::uint32_t>(image.height)); // positive: bottom-up
	writeU16(out, 1);
	writeU16(out, 24);
	writeU32(out, 0);        // BI_RGB
	writeU32(out, dataSize);
	writeU32(out, 2835);     // 72 DPI
	writeU32(out, 2835);
	writeU32(out, 0);
	writeU32(out, 0);

	for (int y = 0; y < image.height; ++y) {
		size_t written = 0;
		for (int x = 0; x < image.width; ++x) {
			const size_t i = (static_cast<size_t>(y) * image.width + x) * 4;
			out.push_back(image.pixels[i + 2]);
			out.push_back(image.pixels[i + 1]);
			out.push_back(image.pixels[i + 0]);
			written += 3;
		}
		for (; written < rowSize; ++written)
			out.push_back(0);
	}

	std::ofstream file(path, std::ios::binary);
	if (!file)
		return false;
	file.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
	return static_cast<bool>(file);
}

}
