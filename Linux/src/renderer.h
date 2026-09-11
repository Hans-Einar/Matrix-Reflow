#pragma once
#include "gl_resources.h"
#include "font_atlas.h"
#include "mmcore.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace reflow {
struct RenderView {
    // Column-major OpenGL matrix; camera/projection ownership stays with caller.
    std::array<float, 16> view_projection{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    std::array<float, 3> camera_right{1,0,0}, camera_up{0,1,0}, camera_position{0,0,1};
    float glyph_half = 36, time = 0;
    std::array<float, 3> fog{0,34,186};
    bool textured = true, wireframe = false, extra_contrast_heads = false;
};
class Renderer {
public:
    explicit Renderer(const FontAtlas& atlas = build_atlas(), bool double_buffered = true);
    void resize(int width, int height);
    void draw_control();
    void draw_instances(const MMGlyphInstance* instances, std::size_t count, const RenderView& view);
    // Capture the same final composite offscreen, independent of X11 occlusion.
    // Top row first; capture target exists only for the duration of the call.
    std::vector<std::uint8_t> read_rgba();
    void write_ppm(const std::string& path);
private:
    void begin_scene();
    void composite(GLuint target = 0);
    int width_ = 0, height_ = 0;
    GLenum output_buffer_ = GL_BACK;
    GlObject scene_{GlKind::Texture}, fbo_{GlKind::Framebuffer}, fullscreen_vao_{GlKind::VertexArray};
    GlObject atlas_{GlKind::Texture}, instance_buffer_{GlKind::Buffer}, glyph_vao_{GlKind::VertexArray};
    int atlas_columns_, atlas_rows_;
    Program control_, composite_, glyph_;
};
}
