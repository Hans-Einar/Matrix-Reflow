#include "renderer.h"
#include "fullscreen_vertex.h"
#include "control_fragment.h"
#include "composite_fragment.h"
#include "glyph_vertex.h"
#include "glyph_fragment.h"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace reflow {
Renderer::Renderer(const FontAtlas& atlas)
    : atlas_columns_(atlas.columns), atlas_rows_(atlas.rows),
      control_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
               reinterpret_cast<const char*>(resources::control_fragment), "control"),
      composite_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
                 reinterpret_cast<const char*>(resources::composite_fragment), "composite"),
      glyph_(reinterpret_cast<const char*>(resources::glyph_vertex),
             reinterpret_cast<const char*>(resources::glyph_fragment), "glyph") {
    static_assert(sizeof(MMGlyphInstance) == 40);
    static_assert(offsetof(MMGlyphInstance, px) == 0);
    static_assert(offsetof(MMGlyphInstance, flipX) == 16);
    static_assert(offsetof(MMGlyphInstance, cr) == 28);
    if (atlas.width < 1 || atlas.height < 1 || atlas.columns < 1 || atlas.rows < 1 ||
        atlas.pixels.size() != static_cast<std::size_t>(atlas.width) * atlas.height)
        throw std::runtime_error("Invalid glyph atlas");
    glBindTexture(GL_TEXTURE_2D, atlas_.get());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas.width, atlas.height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 72-pixel cells divide evenly through level 3; avoid mip mixing across cells.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindVertexArray(glyph_vao_.get());
    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer_.get());
    const int sizes[] = {4,3,3};
    const std::size_t offsets[] = {0,16,28};
    for (GLuint i = 0; i < 3; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(MMGlyphInstance),
                              reinterpret_cast<const void*>(offsets[i]));
        glVertexAttribDivisor(i, 1);
    }
    check_gl("Upload font atlas and configure glyph instances");
}

void Renderer::resize(int width, int height) {
    if (width < 0 || height < 0) throw std::runtime_error("Negative render dimensions");
    if (width == width_ && height == height_) return;
    if (!width || !height) { width_ = width; height_ = height; return; }
    GLint max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (width > max_size || height > max_size ||
        static_cast<std::uint64_t>(width) * height > 33554432)
        throw std::runtime_error("Scene exceeds texture limit or 256 MiB scene budget");
    GlObject texture(GlKind::Texture), framebuffer(GlKind::Framebuffer);
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get(), 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("RGBA16F scene framebuffer is incomplete");
    check_gl("Create scene target");
    scene_ = std::move(texture);
    fbo_ = std::move(framebuffer);
    width_ = width; height_ = height;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::begin_scene() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_.get());
    glViewport(0, 0, width_, height_);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}
void Renderer::draw_control() {
    if (!width_ || !height_) return;
    begin_scene();
    glUseProgram(control_.get());
    glBindVertexArray(fullscreen_vao_.get());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    composite();
}
void Renderer::draw_instances(const MMGlyphInstance* instances, std::size_t count, const RenderView& view) {
    if (!width_ || !height_) return;
    if ((!instances && count) || count > (64 * 1024 * 1024) / sizeof(MMGlyphInstance))
        throw std::runtime_error("Glyph instances exceed 64 MiB upload budget or are missing");
    begin_scene();
    glUseProgram(glyph_.get());
    glUniformMatrix4fv(glyph_.uniform("viewProjection"), 1, GL_FALSE, view.view_projection.data());
    glUniform3fv(glyph_.uniform("cameraRight"), 1, view.camera_right.data());
    glUniform3fv(glyph_.uniform("cameraUp"), 1, view.camera_up.data());
    glUniform3fv(glyph_.uniform("cameraPosition"), 1, view.camera_position.data());
    glUniform3fv(glyph_.uniform("fogParameters"), 1, view.fog.data());
    glUniform1f(glyph_.uniform("glyphHalf"), view.glyph_half);
    glUniform1f(glyph_.uniform("time"), view.time);
    glUniform2f(glyph_.uniform("atlasGrid"), static_cast<float>(atlas_columns_), static_cast<float>(atlas_rows_));
    glUniform1f(glyph_.uniform("textured"), view.textured ? 1.0f : 0.0f);
    glUniform1f(glyph_.uniform("wireframe"), view.wireframe ? 1.0f : 0.0f);
    glUniform1f(glyph_.uniform("extraContrastHeads"), view.extra_contrast_heads ? 1.0f : 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas_.get());
    glUniform1i(glyph_.uniform("atlas"), 0);
    glBindVertexArray(glyph_vao_.get());
    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer_.get());
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(MMGlyphInstance)), instances, GL_STREAM_DRAW);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(count));
    composite();
}
void Renderer::composite(GLuint target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glDrawBuffer(target ? GL_COLOR_ATTACHMENT0 : GL_BACK);
    glViewport(0, 0, width_, height_);
    glDisable(GL_BLEND);
    glUseProgram(composite_.get());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_.get());
    glUniform1i(composite_.uniform("scene"), 0);
    glBindVertexArray(fullscreen_vao_.get());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    check_gl("Render scene/composite");
}
std::vector<std::uint8_t> Renderer::read_rgba() {
    if (!width_ || !height_) throw std::runtime_error("Cannot capture empty viewport");
    std::vector<std::uint8_t> result(static_cast<std::size_t>(width_) * height_ * 4);
    // Reading an obscured default framebuffer is subject to pixel ownership.
    // Reuse the production composite shader, never a separate capture renderer.
    GlObject texture(GlKind::Texture), framebuffer(GlKind::Framebuffer);
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get(), 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Capture framebuffer is incomplete");
    composite(framebuffer.get());
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
    check_gl("Read back frame");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    const std::size_t stride = static_cast<std::size_t>(width_) * 4;
    for (int y = 0; y < height_ / 2; ++y)
        std::swap_ranges(result.begin() + y * stride, result.begin() + (y + 1) * stride,
                         result.begin() + (height_ - y - 1) * stride);
    return result;
}
void Renderer::write_ppm(const std::string& path) {
    const auto rgba = read_rgba();
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (std::size_t i = 0; i < rgba.size(); i += 4)
        out.write(reinterpret_cast<const char*>(rgba.data() + i), 3);
    out.close();
    if (!out) throw std::runtime_error("Cannot write frame: " + path);
}
}
