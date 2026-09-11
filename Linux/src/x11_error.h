#pragma once
#include <X11/Xlib.h>

namespace reflow {
// Scoped, single-threaded Xlib error collection. Nested operations also report
// to their enclosing lifetime guard, including implicit X requests inside GL.
class XErrorTrap {
public:
    explicit XErrorTrap(Display* display) : display_(display), parent_(active_) {
        active_=this; previous_=XSetErrorHandler(handler); XSync(display_,False);
    }
    ~XErrorTrap() { XSync(display_,False); active_=parent_; XSetErrorHandler(previous_); }
    XErrorTrap(const XErrorTrap&)=delete;
    XErrorTrap& operator=(const XErrorTrap&)=delete;
    int error() { XSync(display_,False); return error_; }
private:
    static int handler(Display* display,XErrorEvent* e) {
        bool handled=false;
        XErrorHandler fallback=nullptr;
        for(auto* trap=active_;trap;trap=trap->parent_) {
            fallback=trap->previous_;
            if(trap->display_==display) {trap->error_=e->error_code;handled=true;}
        }
        return !handled && fallback ? fallback(display,e) : 0;
    }
    inline static XErrorTrap* active_=nullptr;
    int error_=0;
    Display* display_;
    XErrorTrap* parent_;
    XErrorHandler previous_;
};
}
