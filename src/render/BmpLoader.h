#pragma once

#include <optional>
#include <string>

#include "Image.h"

// Hand-written reader/writer for Windows BMP files (no image library).
//
// File layout:
//   BITMAPFILEHEADER (14 bytes)  "BM", file size, reserved, offset to pixel data
//   BITMAPINFOHEADER (40+ bytes) width, height (negative = rows stored top-down), planes = 1,
//                                bits per pixel (24 or 32), compression (0 = BI_RGB, 3 = BITFIELDS)
//   pixel rows                   each row padded to a multiple of 4 bytes, colours stored as B,G,R(,A),
//                                rows stored BOTTOM-UP when height > 0
namespace Bmp {

std::optional<Image> Load(const std::string& path);
bool Save(const std::string& path, const Image& image); // writes 24-bit BI_RGB

}
