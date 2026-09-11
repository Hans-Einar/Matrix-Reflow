#include "crt.h"
#include "fullscreen_vertex.h"
#include "crt_fragment.h"
#include <cstdint>
#include <stdexcept>
namespace reflow {
CrtPass::CrtPass()
    : program_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
               reinterpret_cast<const char*>(resources::crt_fragment),"CRT") {}
void CrtPass::resize(int width,int height) {
    if(input_ && input_->width==width && input_->height==height) return;
    GLint max_size=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);
    if(width<1 || height<1 || width>max_size || height>max_size ||
       static_cast<std::uint64_t>(width)*height>33554432)
        throw std::runtime_error("Invalid CRT target dimensions");
    input_=std::make_unique<Bloom::Level>(width,height);
}
void CrtPass::draw(GLuint target,GLenum output_buffer,float time,bool identity) {
    if(!input_ || target==input_->framebuffer.get()) throw std::runtime_error("Invalid CRT output/texture feedback");
    glBindFramebuffer(GL_FRAMEBUFFER,target);
    glDrawBuffer(target?GL_COLOR_ATTACHMENT0:output_buffer);
    glViewport(0,0,input_->width,input_->height);
    glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_FRAMEBUFFER_SRGB);
    glUseProgram(program_.get());
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,input_->texture.get());
    glUniform1i(program_.uniform("source"),0);
    glUniform2f(program_.uniform("resolution"),input_->width,input_->height);
    glUniform1f(program_.uniform("time"),time);
    glUniform1i(program_.uniform("identity"),identity);
    glBindVertexArray(vao_.get());glDrawArrays(GL_TRIANGLES,0,3);
    check_gl("CRT output");
}
}
