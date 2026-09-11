#include "x11_host.h"
#include "x11_error.h"
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <memory>
#include <stdexcept>

namespace reflow {
X11Host::X11Host(int width, int height, bool visible) : width_(width), height_(height) {
    try {
        if (width < 1 || height < 1 || width > 16384 || height > 16384)
            throw std::runtime_error("Window dimensions must be 1..16384");
        display_ = XOpenDisplay(nullptr);
        if (!display_) throw std::runtime_error("Cannot open X11 display; check DISPLAY and Xauthority");
        int major = 0, minor = 0;
        if (!glXQueryVersion(display_, &major, &minor) || major < 1 || (major == 1 && minor < 3))
            throw std::runtime_error("GLX 1.3 or newer is required");
        const int screen = DefaultScreen(display_);
        const int attributes[] = {
            GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
            GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
            GLX_DOUBLEBUFFER, True, None
        };
        int count = 0;
        std::unique_ptr<GLXFBConfig, decltype(&XFree)> configs(
            glXChooseFBConfig(display_, screen, attributes, &count), XFree);
        if (!configs || !count) throw std::runtime_error("No compatible double-buffered GLX visual");
        config_ = configs.get()[0];
        // Prefer the existing desktop visual, avoiding an ARGB top-level window.
        for (int i = 0; i < count; ++i) {
            int visual = 0;
            glXGetFBConfigAttrib(display_, configs.get()[i], GLX_VISUAL_ID, &visual);
            if (static_cast<VisualID>(visual) == XVisualIDFromVisual(DefaultVisual(display_, screen))) {
                config_ = configs.get()[i];
                break;
            }
        }
        std::unique_ptr<XVisualInfo, decltype(&XFree)> visual(
            glXGetVisualFromFBConfig(display_, config_), XFree);
        if (!visual) throw std::runtime_error("GLX visual lookup failed");
        colormap_ = XCreateColormap(display_, RootWindow(display_, screen), visual->visual, AllocNone);
        XSetWindowAttributes attrs{};
        attrs.colormap = colormap_;
        attrs.background_pixel = 0;
        attrs.border_pixel = 0;
        attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
        window_ = XCreateWindow(display_, RootWindow(display_, screen), 0, 0,
            width, height, 0, visual->depth, InputOutput, visual->visual,
            CWColormap | CWBackPixel | CWBorderPixel | CWEventMask, &attrs);
        if (!window_) throw std::runtime_error("Cannot create Matrix Reflow window");
        XStoreName(display_, window_, "Matrix Reflow");
        XClassHint hint{};
        hint.res_name = const_cast<char*>("matrix-reflow");
        hint.res_class = const_cast<char*>("MatrixReflow");
        XSetClassHint(display_, window_, &hint);
        delete_window_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &delete_window_, 1);
        if (visible) XMapWindow(display_, window_);
        XSync(display_, False);
    } catch (...) { release(); throw; }
}
X11Host::~X11Host() { release(); }
X11Host::X11Host(Window borrowed_window) : width_(0),height_(0),owns_window_(false) {
    try {
        if(!borrowed_window) throw std::runtime_error("Host window ID must be nonzero");
        display_=XOpenDisplay(nullptr);
        if(!display_) throw std::runtime_error("Cannot open X11 display");
        XWindowAttributes attrs{};
        {
            XErrorTrap trap(display_);
            const bool found=XGetWindowAttributes(display_,borrowed_window,&attrs);
            if(trap.error() || !found) throw std::runtime_error("Host window no longer exists");
        }
        if(attrs.c_class!=InputOutput || borrowed_window==attrs.root)
            throw std::runtime_error("Expected a drawable host window, never the desktop root");
        const int screen=XScreenNumberOfScreen(attrs.screen);
        int count=0;
        std::unique_ptr<GLXFBConfig,decltype(&XFree)> configs(glXGetFBConfigs(display_,screen,&count),XFree);
        if(!configs) throw std::runtime_error("Host display has no GLX framebuffer configurations");
        for(int i=0;i<count;++i) {
            int visual=0,drawable=0,render=0,doubled=0;
            glXGetFBConfigAttrib(display_,configs.get()[i],GLX_VISUAL_ID,&visual);
            glXGetFBConfigAttrib(display_,configs.get()[i],GLX_DRAWABLE_TYPE,&drawable);
            glXGetFBConfigAttrib(display_,configs.get()[i],GLX_RENDER_TYPE,&render);
            glXGetFBConfigAttrib(display_,configs.get()[i],GLX_DOUBLEBUFFER,&doubled);
            if(static_cast<VisualID>(visual)==XVisualIDFromVisual(attrs.visual) &&
               (drawable&GLX_WINDOW_BIT) && (render&GLX_RGBA_BIT)) {
                config_=configs.get()[i];if(doubled) break;
            }
        }
        if(!config_) throw std::runtime_error("Host visual is incompatible with GLX RGBA rendering");
        window_=borrowed_window;width_=attrs.width;height_=attrs.height;
        {
            XErrorTrap trap(display_);
            XSelectInput(display_,window_,StructureNotifyMask|ExposureMask);
            if(trap.error()) throw std::runtime_error("Host window vanished while attaching");
        }
    } catch(...) {release();throw;}
}
void X11Host::release() {
    if (!display_) return;
    {
        XErrorTrap trap(display_);
        if (window_ && owns_window_) XDestroyWindow(display_, window_);
        if (colormap_) XFreeColormap(display_, colormap_);
    }
    XCloseDisplay(display_);
    display_ = nullptr;
    window_ = 0;
}
bool X11Host::poll() {
    while (XPending(display_)) {
        XEvent event{};
        XNextEvent(display_, &event);
        if (event.xany.window != window_) continue;
        if (event.type == ConfigureNotify) {
            width_ = event.xconfigure.width;
            height_ = event.xconfigure.height;
        } else if (event.type == DestroyNotify) {
            window_ = 0;
            alive_ = false;
        } else if (owns_window_ && event.type == ClientMessage && event.xclient.format == 32 &&
                   event.xclient.message_type == XInternAtom(display_, "WM_PROTOCOLS", False) &&
                   static_cast<Atom>(event.xclient.data.l[0]) == delete_window_) {
            alive_ = false;
        } else if (owns_window_ && event.type == KeyPress && XLookupKeysym(&event.xkey, 0) == XK_Escape) {
            alive_ = false;
        }
    }
    return alive_;
}
void X11Host::resize(int width, int height) {
    if(!owns_window_) throw std::runtime_error("A borrowed window is resized by its owner");
    if (width < 1 || height < 1 || width > 16384 || height > 16384)
        throw std::runtime_error("Window dimensions must be 1..16384");
    XResizeWindow(display_, window_, width, height);
    XSync(display_, False);
}
}
