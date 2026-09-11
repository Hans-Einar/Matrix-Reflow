#pragma once
#include <epoxy/glx.h>
#include <string>

namespace reflow {
// Does not own the Display or X11 window. Destroy before their owner.
class GlxContext {
public:
    GlxContext(Display* display, GLXFBConfig config, Window window);
    ~GlxContext();
    GlxContext(const GlxContext&) = delete;
    GlxContext& operator=(const GlxContext&) = delete;
    bool present();
    bool double_buffered() const { return double_buffered_; }
    std::string description() const;
private:
    void release();
    Display* display_;
    GLXContext context_ = nullptr;
    GLXWindow drawable_ = 0;
    bool double_buffered_ = true;
};
}
