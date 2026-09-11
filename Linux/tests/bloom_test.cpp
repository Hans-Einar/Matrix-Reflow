#include "bloom.h"
#include "renderer.h"
#include <limits>
#include "glx_context.h"
#include "x11_host.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace reflow;
static void require(bool b,const char* message) {if(!b) throw std::runtime_error(message);}
static std::vector<float> read(const Bloom::Level& level) {
    std::vector<float> data(level.width*level.height*4);
    glBindFramebuffer(GL_FRAMEBUFFER,level.framebuffer.get());
    glReadPixels(0,0,level.width,level.height,GL_RGBA,GL_FLOAT,data.data());return data;
}
int main() try {
    X11Host host(128,96,false);GlxContext context(host.display(),host.config(),host.window());
    Bloom bloom;GlObject source(GlKind::Texture);
    auto upload=[&](int w,int h,const std::vector<float>& pixels) {
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,source.get());
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_FLOAT,pixels.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    };
    for(auto size : {std::pair{128,96}, {3,5}, {1,1}, {192,108}, {1,7}}) {
        const auto [w,h]=size;
        for(float value : {.1f,.576f,1.0f}) {
            upload(w,h,std::vector<float>(w*h*4,value));bloom.extract(source.get(),w,h);
            int bw=w,bh=h;
            for(int i=0;i<5;++i) {
                bw=std::max(1,bw/2);bh=std::max(1,bh/2);
                const auto& level=bloom.level(i);
                require(level.width==bw && level.height==bh,"mip dimensions");
                auto data=read(level);
                const float expected=value==.1f?0:value==1?1.5f:.432f;
                for(std::size_t p=0;p<data.size();p+=4)
                    require(std::abs(data[p]-expected)<.004f && data[p+3]==1,"threshold/normalized kernel");
            }
            bloom.accumulate();
            const float expected=value==.1f?0:value==1?7.5f:2.16f;
            require(std::abs(read(bloom.level(0))[0]-expected)<.02f,"additive levels lost existing contribution");
        }
    }
    std::vector<float> point(128*96*4,0);
    for(int y=44;y<52;++y) for(int x=60;x<68;++x) point[(y*128+x)*4+1]=4;
    upload(128,96,point);bloom.extract(source.get(),128,96);
    float previous=100;
    for(int i=0;i<5;++i) {
        auto data=read(bloom.level(i));float peak=0;int lit=0;
        for(std::size_t p=1;p<data.size();p+=4) {peak=std::max(peak,data[p]);lit+=data[p]>0;}
        require(peak>0 && peak<=previous+.001f && lit>0,"bright point propagation/detail reduction");previous=peak;
    }
    Renderer renderer;renderer.resize(128,96);
    auto render=[&](PostSettings settings) {
        renderer.postprocess(settings);renderer.draw_control();return renderer.read_rgba();
    };
    const auto raw=render({false,false,0,0,2});
    const auto off=render({true,false,.9f,0,2});
    require(off==render({true,true,0,0,2}),"zero intensity lost scene");
    require(off==render({true,true,.9f,1,0}),"startup effect easing");
    const auto low=render({true,true,.2f,0,2});
    const auto high=render({true,true,.9f,0,2});
    require(high==renderer.read_rgba(),"capture accumulated bloom twice");
    int brighter=0;
    for(std::size_t i=0;i<high.size();++i) {
        if(i%4==3) require(high[i]==255 && off[i]==255,"opaque output");
        else {require(high[i]>=low[i] && low[i]>=off[i],"intensity monotonicity");brighter+=high[i]>off[i];}
    }
    require(brighter>100,"bloom has no visible contribution");
    const std::size_t center=(48*128+64)*4;
    require(raw[center+2]==off[center+2],"unexpected gamma/tone map at center");
    const auto warped=render({true,true,.9f,1,2});
    for(auto pixel : {0,127,95*128,96*128-1})
        for(int c=0;c<3;++c) require(warped[pixel*4+c]==0,"clamped corner streak");
    bool invalid=false;
    try {renderer.postprocess({true,true,std::numeric_limits<float>::quiet_NaN(),0,1});}
    catch(const std::exception&) {invalid=true;}
    require(invalid,"NaN intensity accepted");
    for(auto size : {std::pair{1,1},{3,5},{128,96}}) {
        renderer.resize(size.first,size.second);render({true,true,.9f,1,2});
        render({false,false,0,0,2});render({true,true,.9f,0,2});
    }
    check_gl("bloom test");
    std::cout<<"Threshold, five normalized levels, bright-point spread and tiny/odd resizes OK\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
