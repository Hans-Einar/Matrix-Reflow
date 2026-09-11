#pragma once
#include <epoxy/gl.h>
#include <string>

namespace reflow {
enum class GlKind { Texture, Framebuffer, Buffer, VertexArray };
// Objects must be destroyed while their context is current.
class GlObject {
public:
    explicit GlObject(GlKind kind);
    ~GlObject();
    GlObject(const GlObject&) = delete;
    GlObject& operator=(const GlObject&) = delete;
    GlObject(GlObject&& other) noexcept;
    GlObject& operator=(GlObject&& other) noexcept;
    GLuint get() const { return id_; }
private:
    void release();
    GlKind kind_;
    GLuint id_ = 0;
};
class Program {
public:
    Program(const char* vertex, const char* fragment, const std::string& label);
    ~Program();
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    GLuint get() const { return id_; }
    GLint uniform(const char* name) const;
private:
    GLuint id_ = 0;
};
void check_gl(const char* operation);
}
