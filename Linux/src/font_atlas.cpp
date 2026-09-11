#include "font_atlas.h"
#include "glyph_table.h"
#include "matrix_font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace reflow {
namespace {
void check(FT_Error error, const char* operation) {
    if (error) throw std::runtime_error(std::string(operation) +
                                       " (FreeType error " + std::to_string(error) + ")");
}
}
void copy_gray_bitmap(const unsigned char* top, int width, int height, int pitch,
                      FontAtlas& atlas, int x, int y) {
    if (!top || width < 0 || height < 0 ||
        static_cast<long long>(pitch) * (pitch < 0 ? -1 : 1) < width ||
        x < 0 || y < 0 || x > atlas.width || y > atlas.height ||
        width > atlas.width - x || height > atlas.height - y ||
        atlas.pixels.size() != static_cast<std::size_t>(atlas.width) * atlas.height)
        throw std::runtime_error("Invalid font bitmap bounds/pitch");
    for (int row = 0; row < height; ++row) {
        const auto* src = top + static_cast<std::ptrdiff_t>(row) * pitch;
        std::copy_n(src, width, atlas.pixels.begin() + (y + row) * atlas.width + x);
    }
}

FontAtlas build_atlas(const unsigned char* bytes, std::size_t size) {
    if (!bytes || size == 0 || size > static_cast<std::size_t>(std::numeric_limits<FT_Long>::max()))
        throw std::runtime_error("Missing or oversized Matrix font data");
    FT_Library library_raw = nullptr;
    check(FT_Init_FreeType(&library_raw), "Initialize FreeType");
    std::unique_ptr<std::remove_pointer_t<FT_Library>, decltype(&FT_Done_FreeType)>
        library(library_raw, FT_Done_FreeType);
    FT_Face face_raw = nullptr;
    check(FT_New_Memory_Face(library.get(), bytes, static_cast<FT_Long>(size), 0, &face_raw),
          "Load embedded Matrix font");
    std::unique_ptr<std::remove_pointer_t<FT_Face>, decltype(&FT_Done_Face)>
        face(face_raw, FT_Done_Face);
    check(FT_Select_Charmap(face.get(), FT_ENCODING_UNICODE), "Select Unicode charmap");
    FontAtlas atlas;
    atlas.pixels.resize(static_cast<std::size_t>(atlas.width) * atlas.height);
    check(FT_Set_Pixel_Sizes(face.get(), 0, 59), "Set Matrix font size"); // 72 * 0.82
    const int ascender = static_cast<int>(face->size->metrics.ascender / 64);
    const int descender = static_cast<int>(face->size->metrics.descender / 64);
    const int baseline = (atlas.cell_size - (ascender - descender)) / 2 + ascender;
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        const FT_UInt index = FT_Get_Char_Index(face.get(), glyphs[i]);
        if (!index) throw std::runtime_error("Missing Matrix glyph at index " + std::to_string(i));
        check(FT_Load_Glyph(face.get(), index, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL),
              "Rasterize Matrix glyph");
        const auto& slot = *face->glyph;
        const auto& bitmap = slot.bitmap;
        if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.num_grays != 256 ||
            !bitmap.width || !bitmap.rows)
            throw std::runtime_error("Expected nonempty 8-bit Matrix glyph");
        const int left = (atlas.cell_size - static_cast<int>(slot.advance.x / 64)) / 2 + slot.bitmap_left;
        const int top = baseline - slot.bitmap_top;
        // Keep a black border for linear filtering; fail rather than silently clip.
        if (left < 1 || top < 1 || left + static_cast<int>(bitmap.width) >= atlas.cell_size ||
            top + static_cast<int>(bitmap.rows) >= atlas.cell_size)
            throw std::runtime_error("Matrix glyph does not fit cell: " + std::to_string(i));
        const int x = static_cast<int>(i % atlas.columns) * atlas.cell_size + left;
        const int y = static_cast<int>(i / atlas.columns) * atlas.cell_size + top;
        copy_gray_bitmap(bitmap.buffer, static_cast<int>(bitmap.width),
                         static_cast<int>(bitmap.rows), bitmap.pitch, atlas, x, y);
    }
    return atlas;
}

FontAtlas build_atlas() {
    return build_atlas(resources::matrix_font, resources::matrix_font_size);
}

void FontAtlas::write_pgm(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    out << "P5\n" << width << ' ' << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    out.close();
    if (!out) throw std::runtime_error("Cannot write atlas: " + path);
}
}
