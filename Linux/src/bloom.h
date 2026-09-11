#pragma once
#include "gl_resources.h"
#include <vector>

namespace reflow {
class Bloom {
public:
    struct Level {
        int width, height;
        GlObject texture{GlKind::Texture}, framebuffer{GlKind::Framebuffer};
        Level(int w,int h);
    };
    Bloom();
    void resize(int width,int height);
    void extract(GLuint scene,int width,int height);
    const Level& level(int index) const { return levels_.at(index); }
private:
    void pass(const Program& program,GLuint source,int width,int height,const Level& target);
    int width_=0,height_=0;
    std::vector<Level> levels_;
    GlObject vao_{GlKind::VertexArray};
    Program threshold_,downsample_;
};
}
