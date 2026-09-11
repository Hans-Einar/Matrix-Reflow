#pragma once
#include <epoxy/glx.h>

namespace reflow {
// Owns only its own window/display. Borrowed-window support comes in iteration 2.
class X11Host {
public:
    X11Host(int width, int height, bool visible = true);
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
private:
    void release();
    Display* display_ = nullptr;
    Window window_ = 0;
    Colormap colormap_ = 0;
    GLXFBConfig config_ = nullptr;
    Atom delete_window_ = 0;
    int width_, height_;
    bool alive_ = true;
};
}
