#include "renderer.h"
#include "fullscreen_vertex.h"
#include "control_fragment.h"
#include "composite_fragment.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace reflow {
Renderer::Renderer()
    : control_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
               reinterpret_cast<const char*>(resources::control_fragment), "control"),
      composite_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
                 reinterpret_cast<const char*>(resources::composite_fragment), "composite") {}

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
void Renderer::composite() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
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
std::vector<std::uint8_t> Renderer::read_rgba() const {
    if (!width_ || !height_) throw std::runtime_error("Cannot capture empty viewport");
    std::vector<std::uint8_t> result(static_cast<std::size_t>(width_) * height_ * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
    check_gl("Read back frame");
    const std::size_t stride = static_cast<std::size_t>(width_) * 4;
    for (int y = 0; y < height_ / 2; ++y)
        std::swap_ranges(result.begin() + y * stride, result.begin() + (y + 1) * stride,
                         result.begin() + (height_ - y - 1) * stride);
    return result;
}
void Renderer::write_ppm(const std::string& path) const {
    const auto rgba = read_rgba();
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (std::size_t i = 0; i < rgba.size(); i += 4)
        out.write(reinterpret_cast<const char*>(rgba.data() + i), 3);
    out.close();
    if (!out) throw std::runtime_error("Cannot write frame: " + path);
}
}
