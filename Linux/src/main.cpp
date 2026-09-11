#include "mmcore.h"
#include "font_atlas.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) try {
    if (argc == 3 && std::string(argv[1]) == "--dump-atlas") {
        reflow::build_atlas().write_pgm(argv[2]);
        return 0;
    }
    if (argc > 1 && (argc != 2 || std::string(argv[1]) != "--help")) {
        std::cerr << "Unknown arguments; use --help\n";
        return 1;
    }
    const auto settings = mm_settings_default();
    std::cout << "Matrix Reflow Linux 0.1.0: build foundation\n"
              << "Core default density: " << settings.density << '\n'
              << "--dump-atlas FILE.pgm  Rasterize all 57 Matrix glyphs (no display required)\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "matrix-reflow: " << e.what() << '\n';
    return 1;
}
