#include "gl_resources.h"
#include <stdexcept>
#include <utility>
#include <vector>

namespace reflow {
GlObject::GlObject(GlKind kind) : kind_(kind) {
    switch (kind_) {
    case GlKind::Texture: glGenTextures(1, &id_); break;
    case GlKind::Framebuffer: glGenFramebuffers(1, &id_); break;
    case GlKind::Buffer: glGenBuffers(1, &id_); break;
    case GlKind::VertexArray: glGenVertexArrays(1, &id_); break;
    }
}
void GlObject::release() {
    if (!id_) return;
    switch (kind_) {
    case GlKind::Texture: glDeleteTextures(1, &id_); break;
    case GlKind::Framebuffer: glDeleteFramebuffers(1, &id_); break;
    case GlKind::Buffer: glDeleteBuffers(1, &id_); break;
    case GlKind::VertexArray: glDeleteVertexArrays(1, &id_); break;
    }
    id_ = 0;
}
GlObject::~GlObject() { release(); }
GlObject::GlObject(GlObject&& other) noexcept : kind_(other.kind_), id_(std::exchange(other.id_, 0)) {}
GlObject& GlObject::operator=(GlObject&& other) noexcept {
    if (this != &other) { release(); kind_ = other.kind_; id_ = std::exchange(other.id_, 0); }
    return *this;
}
namespace {
struct Shader {
    GLuint id;
    explicit Shader(GLenum type) : id(glCreateShader(type)) {}
    ~Shader() { glDeleteShader(id); }
    void compile(const char* source, const std::string& label) {
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);
        GLint ok = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint length = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> log(static_cast<std::size_t>(length) + 1);
            glGetShaderInfoLog(id, length, nullptr, log.data());
            throw std::runtime_error(label + ": " + log.data());
        }
    }
};
}
Program::Program(const char* vertex, const char* fragment, const std::string& label) {
    Shader vs(GL_VERTEX_SHADER), fs(GL_FRAGMENT_SHADER);
    vs.compile(vertex, label + " vertex shader");
    fs.compile(fragment, label + " fragment shader");
    id_ = glCreateProgram();
    glAttachShader(id_, vs.id);
    glAttachShader(id_, fs.id);
    glLinkProgram(id_);
    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<std::size_t>(length) + 1);
        glGetProgramInfoLog(id_, length, nullptr, log.data());
        glDeleteProgram(id_);
        id_ = 0;
        throw std::runtime_error(label + " link: " + log.data());
    }
}
Program::~Program() { if (id_) glDeleteProgram(id_); }
GLint Program::uniform(const char* name) const {
    const GLint result = glGetUniformLocation(id_, name);
    if (result < 0) throw std::runtime_error(std::string("Missing shader uniform: ") + name);
    return result;
}
void check_gl(const char* operation) {
    const auto error = glGetError();
    if (error != GL_NO_ERROR)
        throw std::runtime_error(std::string(operation) + " (GL error " + std::to_string(error) + ")");
}
}
