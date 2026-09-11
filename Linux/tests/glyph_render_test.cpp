#include "font_atlas.h"
#include "glyph_table.h"
#include "glx_context.h"
#include "renderer.h"
#include "x11_host.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main() try {
    using namespace reflow;
    const auto atlas = build_atlas();
    X11Host host(atlas.width, atlas.height, false);
    GlxContext context(host.display(), host.config(), host.window());
    Renderer renderer(atlas);
    renderer.resize(atlas.width, atlas.height);
    RenderView view;
    view.view_projection[0] = 2.0f / atlas.width;
    view.view_projection[5] = 2.0f / atlas.height;
    view.glyph_half = atlas.cell_size / 2.0f;
    std::vector<MMGlyphInstance> instances;
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        instances.push_back({static_cast<float>((i % 16) * 72 + 36) - atlas.width / 2.0f,
                             atlas.height / 2.0f - static_cast<float>((i / 16) * 72 + 36), 0,
                             static_cast<float>(i), 1, 1, .8f, .15f, .7f, .3f});
    }
    int worst = 0;
    for (int flip = 0; flip < 4; ++flip) {
        for (auto& instance : instances) {
            instance.flipX = flip & 1 ? -1.0f : 1.0f;
            instance.flipY = flip & 2 ? -1.0f : 1.0f;
        }
        renderer.draw_instances(instances.data(), instances.size(), view);
        const auto pixels = renderer.read_rgba();
        for (int y = 0; y < atlas.height; ++y) {
            for (int x = 0; x < atlas.width; ++x) {
                const int cell = (y / 72) * 16 + x / 72;
                const auto output = static_cast<std::size_t>(y * atlas.width + x) * 4;
                require(pixels[output+3] == 255, "final alpha must remain opaque");
                const int sx = (x / 72) * 72 + (flip & 1 ? 71-x%72 : x%72);
                const int sy = (y / 72) * 72 + (flip & 2 ? 71-y%72 : y%72);
                float coverage = atlas.pixels[sy * atlas.width + sx] / 255.0f;
                if (coverage < .01f) coverage = 0;
                const float gain = cell < 57 ? std::pow(.8f, 1.2f) *
                    (.9f + .1f * std::sin(instances[cell].py * .1f)) * 1.5f : 0;
                const float rgb[] = {.15f,.7f,.3f};
                for (int c = 0; c < 3; ++c) {
                    const int expected = static_cast<int>(std::lround(std::min(1.0f, coverage * gain * rgb[c]) * 255));
                    worst = std::max(worst, std::abs(expected - pixels[output+c]));
                }
            }
        }
        context.present();
    }
    // Floating-point interpolation, half-float scene storage and output dithering
    // permit a few 8-bit levels; wrong atlas rows/flips/colors differ by far more.
    require(worst <= 4, "rendered glyphs differ from atlas/color/flip reference");
    renderer.draw_instances(nullptr, 0, view);
    const auto empty = renderer.read_rgba();
    for (std::size_t i = 0; i < empty.size(); i += 4)
        require(empty[i] == 0 && empty[i+1] == 0 && empty[i+2] == 0 && empty[i+3] == 255,
                "empty scene must be opaque black");
    bool rejected = false;
    try { renderer.draw_instances(nullptr, 1, view); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "missing instances accepted");
    std::cout << "57 GPU glyphs x 4 flip modes match CPU atlas; maximum channel error " << worst
              << "; colors, alpha, empty scene and missing buffer OK\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "glyph-render-test: " << e.what() << '\n';
    return 1;
}
