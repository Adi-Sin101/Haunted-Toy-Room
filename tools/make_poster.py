"""Generates assets/textures/poster.bmp - a space-ranger poster for the toy room wall.

Pure Python (no image library): pixels are computed directly and written as a 24-bit BMP,
the same format the C++ program reads back with its own loader (src/render/BmpLoader.cpp).

    python tools/make_poster.py
"""
import math
import os
import random
import struct

W, H = 256, 384

# 5x7 bitmap font for the few letters we need (1 = pixel on), rows top to bottom.
FONT = {
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "F": ["11111", "10000", "11110", "10000", "10000", "10000", "10000"],
    "Y": ["10001", "01010", "00100", "00100", "00100", "00100", "00100"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "B": ["11110", "10001", "11110", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "11110", "10000", "10000", "10000", "11111"],
    " ": ["00000"] * 7,
}


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def main():
    random.seed(7)
    # img[y][x], y = 0 is the TOP row while drawing
    img = [[(0.0, 0.0, 0.0)] * W for _ in range(H)]

    # Background: vertical gradient deep purple -> night blue
    for y in range(H):
        c = lerp((0.30, 0.08, 0.45), (0.03, 0.04, 0.18), y / H)
        img[y] = [c] * W

    # Stars
    for _ in range(160):
        x, y = random.randrange(W), random.randrange(H)
        b = 0.5 + 0.5 * random.random()
        img[y][x] = (b, b, b)

    # Ringed planet
    cx, cy, r = 170, 120, 46
    for y in range(H):
        for x in range(W):
            dx, dy = x - cx, y - cy
            if dx * dx + dy * dy <= r * r:
                band = 0.5 + 0.5 * math.sin(dy * 0.35)
                img[y][x] = lerp((0.2, 0.75, 0.35), (0.1, 0.45, 0.25), band)
            # ring: ellipse band, tilted
            rx = dx * math.cos(0.35) + dy * math.sin(0.35)
            ry = -dx * math.sin(0.35) + dy * math.cos(0.35)
            e = (rx / 80.0) ** 2 + (ry / 16.0) ** 2
            if 0.8 < e < 1.0 and not (dx * dx + dy * dy <= r * r and ry < 0):
                img[y][x] = (0.85, 0.75, 0.45)

    # Rocket (triangle nose + body + fins) flying up-left
    for y in range(H):
        for x in range(W):
            u, v = x - 70, y - 230
            if abs(u) <= 16 and 0 <= v <= 60:
                img[y][x] = (0.92, 0.92, 0.95)
            if 0 > v >= -30 and abs(u) <= 16 * (v + 30) / 30:
                img[y][x] = (0.85, 0.15, 0.15)
            if 40 <= v <= 70 and 16 < abs(u) <= 16 + (v - 40) * 0.8:
                img[y][x] = (0.85, 0.15, 0.15)
            if (u * u + (v - 18) ** 2) <= 49:
                img[y][x] = (0.3, 0.6, 0.95)
            if 60 < v <= 60 + 26 * (1 - abs(u) / 14) and abs(u) < 14:
                img[y][x] = lerp((1.0, 0.9, 0.2), (1.0, 0.35, 0.05), (v - 60) / 26)

    # Text, scaled 2x
    def text(s, top, color, scale=2):
        width = len(s) * 6 * scale
        left = (W - width) // 2
        for i, ch in enumerate(s):
            glyph = FONT[ch]
            for gy, row in enumerate(glyph):
                for gx, bit in enumerate(row):
                    if bit == "1":
                        for sy in range(scale):
                            for sx in range(scale):
                                img[top + gy * scale + sy][left + (i * 6 + gx) * scale + sx] = color

    text("TO INFINITY", 312, (1.0, 0.85, 0.2))
    text("AND BEYOND", 338, (1.0, 0.85, 0.2))

    # White border
    for y in range(H):
        for x in range(W):
            if x < 6 or x >= W - 6 or y < 6 or y >= H - 6:
                img[y][x] = (0.95, 0.95, 0.92)

    # --- write 24-bit BMP (rows bottom-up, BGR, each row padded to 4 bytes) ---
    row_size = (W * 3 + 3) & ~3
    data = bytearray()
    for y in range(H - 1, -1, -1):
        row = bytearray()
        for x in range(W):
            r, g, b = (max(0, min(255, int(c * 255 + 0.5))) for c in img[y][x])
            row += bytes((b, g, r))
        row += b"\x00" * (row_size - len(row))
        data += row
    header = b"BM" + struct.pack("<IHHI", 54 + len(data), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, W, H, 1, 24, 0, len(data), 2835, 2835, 0, 0)

    out = os.path.join(os.path.dirname(__file__), "..", "assets", "textures", "poster.bmp")
    with open(out, "wb") as f:
        f.write(header + info + data)
    print("wrote", os.path.abspath(out))


if __name__ == "__main__":
    main()
