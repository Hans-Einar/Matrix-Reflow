#include "bloom.h"
#include "fullscreen_vertex.h"
#include "bloom_threshold_fragment.h"
#include "bloom_downsample_fragment.h"
#include "bloom_upsample_fragment.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace reflow {
Bloom::Level::Level(int w,int h) : width(w),height(h) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texture.get());
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer.get());
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture.get(),0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete bloom framebuffer");
    check_gl("Create bloom level");
}
Bloom::Bloom()
    : threshold_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
                 reinterpret_cast<const char*>(resources::bloom_threshold_fragment),"bloom threshold"),
      downsample_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
                  reinterpret_cast<const char*>(resources::bloom_downsample_fragment),"bloom downsample"),
      upsample_(reinterpret_cast<const char*>(resources::fullscreen_vertex),
                reinterpret_cast<const char*>(resources::bloom_upsample_fragment),"bloom upsample") {}
void Bloom::resize(int width,int height) {
    if(width<1 || height<1) throw std::runtime_error("Bloom requires positive dimensions");
    if(width==width_ && height==height_) return;
    GLint max_size=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);
    if(width>max_size || height>max_size || static_cast<std::uint64_t>(width)*height>33554432)
        throw std::runtime_error("Bloom exceeds scene dimensions budget");
    std::vector<Level> next;next.reserve(5);
    int w=width,h=height;
    for(int i=0;i<5;++i) {w=std::max(1,w/2);h=std::max(1,h/2);next.emplace_back(w,h);}
    levels_=std::move(next);width_=width;height_=height;
}
void Bloom::pass(const Program& program,GLuint source,int width,int height,const Level& target) {
    if(source==target.texture.get()) throw std::runtime_error("Bloom texture feedback");
    glBindFramebuffer(GL_FRAMEBUFFER,target.framebuffer.get());
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0,0,target.width,target.height);
    glUseProgram(program.get());
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source);
    glUniform1i(program.uniform("source"),0);
    glUniform2f(program.uniform("texel"),1.0f/width,1.0f/height);
    glBindVertexArray(vao_.get());glDrawArrays(GL_TRIANGLES,0,3);
}
void Bloom::extract(GLuint scene,int width,int height) {
    resize(width,height);
    glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);glDisable(GL_FRAMEBUFFER_SRGB);
    pass(threshold_,scene,width,height,levels_[0]);
    for(int i=1;i<5;++i) pass(downsample_,levels_[i-1].texture.get(),levels_[i-1].width,levels_[i-1].height,levels_[i]);
    check_gl("Extract/downsample bloom");
}
void Bloom::accumulate() {
    if(levels_.size()!=5) throw std::runtime_error("Bloom must be extracted before accumulation");
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_ONE,GL_ONE);
    for(int i=3;i>=0;--i)
        pass(upsample_,levels_[i+1].texture.get(),levels_[i+1].width,levels_[i+1].height,levels_[i]);
    glDisable(GL_BLEND);
    check_gl("Additive bloom upsampling");
}

}
