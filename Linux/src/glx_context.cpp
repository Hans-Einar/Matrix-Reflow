#include "glx_context.h"
#include "x11_error.h"
#include <stdexcept>

namespace reflow {
GlxContext::GlxContext(Display* display, GLXFBConfig config, Window window) : display_(display) {
    try {
        int screen = 0, doubled=0;
        glXGetFBConfigAttrib(display_,config,GLX_DOUBLEBUFFER,&doubled);
        double_buffered_=doubled;
        glXGetFBConfigAttrib(display_, config, GLX_SCREEN, &screen);
        if (!epoxy_has_glx_extension(display_, screen, "GLX_ARB_create_context") ||
            !epoxy_has_glx_extension(display_, screen, "GLX_ARB_create_context_profile"))
            throw std::runtime_error("GLX_ARB_create_context/profile required for OpenGL 3.3 core");
        XErrorTrap trap(display_);
        const int attrs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None
        };
        context_ = glXCreateContextAttribsARB(display_, config, nullptr, True, attrs);
        if (trap.error() || !context_) throw std::runtime_error("Cannot create OpenGL 3.3 core context");
        const auto drawable = glXCreateWindow(display_, config, window, nullptr);
        if (trap.error() || !drawable) throw std::runtime_error("Cannot create compatible GLX drawable");
        drawable_=drawable;
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
    XErrorTrap trap(display_);
    if (context_) {
        glXMakeContextCurrent(display_, None, None, nullptr);
        glXDestroyContext(display_, context_);
        context_ = nullptr;
    }
    if (drawable_) { glXDestroyWindow(display_, drawable_); drawable_ = 0; }
}
bool GlxContext::present() {
    XErrorTrap trap(display_);
    if(double_buffered_) glXSwapBuffers(display_,drawable_); else glFlush();
    return trap.error()==0;
}
std::string GlxContext::description() const {
    return std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))) +
        " / OpenGL " + reinterpret_cast<const char*>(glGetString(GL_VERSION));
}
}
