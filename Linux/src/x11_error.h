#pragma once
#include <X11/Xlib.h>

namespace reflow {
// Scoped, single-threaded Xlib error collection. Never nest these scopes.
class XErrorTrap {
public:
    explicit XErrorTrap(Display* display) : display_(display) {
        error_=0; previous_=XSetErrorHandler(handler); XSync(display_,False);
    }
    ~XErrorTrap() { XSync(display_,False); XSetErrorHandler(previous_); }
    int error() { XSync(display_,False); return error_; }
private:
    static int handler(Display*,XErrorEvent* e) { error_=e->error_code;return 0; }
    inline static int error_=0;
    Display* display_;
    XErrorHandler previous_;
};
}
