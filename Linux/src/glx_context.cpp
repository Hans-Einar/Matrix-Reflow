#include "glx_context.h"
#include <stdexcept>

namespace reflow {
namespace {
// Xlib error handlers are process-global. Context setup runs on the sole X11 thread.
class ErrorTrap {
public:
    explicit ErrorTrap(Display* display) : display_(display) {
        XSync(display_, False);
        error_ = 0;
        previous_ = XSetErrorHandler(handler);
    }
    ~ErrorTrap() { XSync(display_, False); XSetErrorHandler(previous_); }
    int error() { XSync(display_, False); return error_; }
private:
    static int handler(Display*, XErrorEvent* e) { error_ = e->error_code; return 0; }
    inline static int error_ = 0;
    Display* display_;
    XErrorHandler previous_;
};
}
GlxContext::GlxContext(Display* display, GLXFBConfig config, Window window) : display_(display) {
    try {
        int screen = 0;
        glXGetFBConfigAttrib(display_, config, GLX_SCREEN, &screen);
        if (!epoxy_has_glx_extension(display_, screen, "GLX_ARB_create_context") ||
            !epoxy_has_glx_extension(display_, screen, "GLX_ARB_create_context_profile"))
            throw std::runtime_error("GLX_ARB_create_context/profile required for OpenGL 3.3 core");
        ErrorTrap trap(display_);
        const int attrs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None
        };
        context_ = glXCreateContextAttribsARB(display_, config, nullptr, True, attrs);
        if (trap.error() || !context_) throw std::runtime_error("Cannot create OpenGL 3.3 core context");
        drawable_ = glXCreateWindow(display_, config, window, nullptr);
        if (trap.error() || !drawable_) throw std::runtime_error("Cannot create compatible GLX drawable");
        if (!glXMakeContextCurrent(display_, drawable_, drawable_, context_) || trap.error())
            throw std::runtime_error("Cannot make GLX context current");
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        if (major < 3 || (major == 3 && minor < 3)) throw std::runtime_error("OpenGL 3.3 required");
        if (epoxy_has_glx_extension(display_, screen, "GLX_EXT_swap_control"))
            glXSwapIntervalEXT(display_, drawable_, 1);
        else if (epoxy_has_glx_extension(display_, screen, "GLX_MESA_swap_control"))
            glXSwapIntervalMESA(1);
    } catch (...) { release(); throw; }
}
GlxContext::~GlxContext() { release(); }
void GlxContext::release() {
    if (context_) {
        glXMakeContextCurrent(display_, None, None, nullptr);
        glXDestroyContext(display_, context_);
        context_ = nullptr;
    }
    if (drawable_) { glXDestroyWindow(display_, drawable_); drawable_ = 0; }
}
void GlxContext::present() { glXSwapBuffers(display_, drawable_); }
std::string GlxContext::description() const {
    return std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))) +
        " / OpenGL " + reinterpret_cast<const char*>(glGetString(GL_VERSION));
}
}
