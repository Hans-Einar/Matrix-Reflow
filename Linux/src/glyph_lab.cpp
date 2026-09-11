#include "glyph_lab.h"
#include "glyph_table.h"
#include <algorithm>

namespace reflow {
std::vector<MMGlyphInstance> glyph_lab_instances() {
    const auto settings = mm_settings_default();
    std::vector<MMGlyphInstance> instances;
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        instances.push_back({-540.0f + 72 * (i % 16), 260.0f - 72 * (i / 16), 0,
                             static_cast<float>(i), 1, 1, 0.8f,
                             settings.mainColorR, settings.mainColorG, settings.mainColorB});
    }
    const int sample[] = {3,4,12,24,29,55};
    const float colors[][3] = {{.05f,.85f,.25f}, {.85f,.85f,.15f}, {.1f,.7f,.85f}, {.85f,.2f,.4f}};
    for (int group = 0; group < 4; ++group) {
        for (int i = 0; i < 6; ++i) {
            const float x = -432.0f + group * 288 + (i - 2.5f) * 44;
            instances.push_back({x, -92, 0, static_cast<float>(sample[i]),
                group & 1 ? -1.0f : 1.0f, group & 2 ? -1.0f : 1.0f,
                .8f, colors[group][0], colors[group][1], colors[group][2]});
            // Second line shows different brightness; slightly offset to separate glyphs.
            instances.push_back({x, -220, 0, static_cast<float>(sample[i]), 1, 1,
                .2f + group * .2f, .05f,.85f,.25f});
        }
    }
    return instances;
}
RenderView glyph_lab_view(int width, int height) {
    RenderView view;
    if (width < 1 || height < 1) return view;
    const float scale = std::min(width / 1152.0f, height / 640.0f);
    view.view_projection[0] = 2.0f * scale / width;
    view.view_projection[5] = 2.0f * scale / height;
    view.glyph_half = 28;
    return view;
}
}
