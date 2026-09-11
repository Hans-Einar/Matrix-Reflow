#pragma once
#include "gl_resources.h"
#include <cstdint>
#include <string>
#include <vector>

namespace reflow {
class Renderer {
public:
    Renderer();
    void resize(int width, int height);
    void draw_control();
    // Read before present(), top row first.
    std::vector<std::uint8_t> read_rgba() const;
    void write_ppm(const std::string& path) const;
private:
    void begin_scene();
    void composite();
    int width_ = 0, height_ = 0;
    GlObject scene_{GlKind::Texture}, fbo_{GlKind::Framebuffer}, fullscreen_vao_{GlKind::VertexArray};
    Program control_, composite_;
};
}
