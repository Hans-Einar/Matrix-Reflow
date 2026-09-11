#pragma once
#include "bloom.h"
#include <memory>
namespace reflow {
// Reuses the FP16 texture/FBO resource used for bloom levels, at full size.
class CrtPass {
public:
    CrtPass();
    void resize(int width,int height);
    const Bloom::Level& input() const { return *input_; }
    void draw(GLuint target,GLenum output_buffer,float time,bool identity);
private:
    std::unique_ptr<Bloom::Level> input_;
    GlObject vao_{GlKind::VertexArray};
    Program program_;
};
}
