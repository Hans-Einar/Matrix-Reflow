#pragma once
#include <epoxy/glx.h>

namespace reflow {
// Owns its connection, but only destroys windows it created itself.
class X11Host {
public:
    X11Host(int width, int height, bool visible = true);
    explicit X11Host(Window borrowed_window);
    ~X11Host();
    X11Host(const X11Host&) = delete;
    X11Host& operator=(const X11Host&) = delete;
    bool poll();
    void resize(int width, int height);
    Display* display() const { return display_; }
    Window window() const { return window_; }
    GLXFBConfig config() const { return config_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool owns_window() const { return owns_window_; }
private:
    void release();
    Display* display_ = nullptr;
    Window window_ = 0;
    Colormap colormap_ = 0;
    GLXFBConfig config_ = nullptr;
    Atom delete_window_ = 0;
    int width_, height_;
    bool alive_ = true;
    bool owns_window_ = true;
};
}
