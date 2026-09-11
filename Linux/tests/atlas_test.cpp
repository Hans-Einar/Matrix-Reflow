#include "font_atlas.h"
#include "glyph_table.h"
#include "mmcore.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void must_fail(F f) {
    bool failed = false;
    try { f(); } catch (const std::runtime_error&) { failed = true; }
    require(failed, "invalid input was accepted");
}
int main() try {
    using namespace reflow;
    static_assert(glyphs[MM_BINARY_GLYPH_ZERO_INDEX] == '0');
    static_assert(glyphs[MM_BINARY_GLYPH_ONE_INDEX] == '1');
    static_assert(glyphs[MM_GLYPH_COLON_INDEX] == ':');
    const auto atlas = build_atlas();
    for (int cell = 0; cell < atlas.columns * atlas.rows; ++cell) {
        unsigned total = 0;
        for (int y = 0; y < atlas.cell_size; ++y)
            for (int x = 0; x < atlas.cell_size; ++x) {
                const int offset = (cell / atlas.columns * atlas.cell_size + y) * atlas.width +
                                   cell % atlas.columns * atlas.cell_size + x;
                const unsigned value = atlas.pixels[offset];
                total += value;
                if (x == 0 || y == 0 || x == atlas.cell_size-1 || y == atlas.cell_size-1)
                    require(value == 0, "glyph bleeds across cell border");
            }
        require((total > 0) == (cell < 57), "empty glyph or nonempty unused cell");
    }
    must_fail([] { build_atlas(nullptr, 0); });
    const unsigned char junk[] = {1,2,3,4};
    must_fail([&] { build_atlas(junk, sizeof(junk)); });
    FontAtlas test;
    test.width = 2; test.height = 2; test.pixels.resize(4);
    const unsigned char positive[] = {1,2,99,3,4,99};
    copy_gray_bitmap(positive, 2, 2, 3, test, 0, 0);
    require(test.pixels == std::vector<std::uint8_t>({1,2,3,4}), "positive padded pitch");
    const unsigned char negative[] = {3,4,99,1,2,99};
    copy_gray_bitmap(negative+3, 2, 2, -3, test, 0, 0);
    require(test.pixels == std::vector<std::uint8_t>({1,2,3,4}), "negative padded pitch");
    must_fail([&] { copy_gray_bitmap(positive, 2, 2, 1, test, 0, 0); });
    must_fail([&] { copy_gray_bitmap(positive, 2, 2, 3, test, 1, 0); });
    std::cout << "57 glyphs, empty borders, stable indices, signed pitch and font errors OK\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "atlas-test: " << e.what() << '\n';
    return 1;
}
