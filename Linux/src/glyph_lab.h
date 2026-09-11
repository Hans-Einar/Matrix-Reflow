#pragma once
#include "renderer.h"

namespace reflow {
std::vector<MMGlyphInstance> glyph_lab_instances();
RenderView glyph_lab_view(int width, int height);
}
