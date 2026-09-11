#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace reflow {
struct FontAtlas {
    int cell_size = 72, columns = 16, rows = 4;
    int width = 1152, height = 288;
    // Top row first, unmirrored; glyph shader handles orientation and flips.
    std::vector<std::uint8_t> pixels;
    void write_pgm(const std::string& path) const;
};
FontAtlas build_atlas();
// Data only needs to remain alive until this function returns.
FontAtlas build_atlas(const unsigned char* bytes, std::size_t size);

// Top points to the first logical row, including for a negative pitch.
void copy_gray_bitmap(const unsigned char* top, int width, int height, int pitch,
                      FontAtlas& atlas, int x, int y);
}
