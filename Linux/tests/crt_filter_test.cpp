#include "crt.h"
#include "glx_context.h"
#include "x11_host.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using RGB=std::array<double,3>;
static double smooth(double lo,double hi,double x) {double t=std::clamp((x-lo)/(hi-lo),0.0,1.0);return t*t*(3-2*t);}
// CPU oracle of the reference HLSL, with top-left pixel coordinates and clamp
// bilinear sampling. Independent of the GLSL and of scene/camera projection.
static RGB reference(const std::vector<RGB>& pixels,int w,int h,int x,int y,double time) {
    auto sample=[&](double sx,int c) {
        int lo=static_cast<int>(std::floor(sx));double f=sx-lo;
        return pixels[y*w+std::clamp(lo,0,w-1)][c]*(1-f)+pixels[y*w+std::clamp(lo+1,0,w-1)][c]*f;
    };
    RGB c{};
    for(int channel=0;channel<3;++channel) {
        const double b=.06*sample(x-2,channel)+.18*sample(x-1,channel)+.52*sample(x,channel)+.18*sample(x+1,channel)+.06*sample(x+2,channel);
        c[channel]=channel==1 ? b : .5*(b+sample(x+(channel==0?.6:-.6),channel));
    }
    const double lum=.299*c[0]+.587*c[1]+.114*c[2], strength=.85*std::clamp(lum*2,0.0,1.0);
    const double phase=std::fmod((x+.5)/2.2,1.0);
    const int selected=phase<1.0/3?0:phase<2.0/3?1:2;
    // At pixel centers, the reference two-pixel scan envelope is exactly 1.
    // The remaining line modulation is its time-dependent sine ripple.
    const double scan=.985+.015*std::sin((y+.5)*1.7+time*.6);
    const double nx=(x+.5)/w-.5,ny=(y+.5)/h-.5;
    const double vignette=1-.22*smooth(.12,.5,nx*nx+ny*ny);
    for(int i=0;i<3;++i) {
        c[i]*=(1+((i==selected?1.08:.82)-1)*strength)*scan;
        c[i]=.02+.98*c[i];c[i]=c[i]/(1+.15*c[i])*vignette;
    }
    return c;
}
int main() try {
    using namespace reflow;
    X11Host host(128,96,false);GlxContext context(host.display(),host.config(),host.window());
    CrtPass crt;int worst=0;
    for(auto size : {std::pair{1,1},{31,17},{128,96},{681,382}}) {
        auto [w,h]=size;crt.resize(w,h);Bloom::Level output(w,h);
        glBindTexture(GL_TEXTURE_2D,output.texture.get());
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        for(int fixture=0;fixture<3;++fixture) {
            std::vector<RGB> pixels(w*h);
            for(int y=0;y<h;++y) for(int x=0;x<w;++x)
                pixels[y*w+x]=fixture==0?RGB{.5,.5,.5}:fixture==1?(x==w/2?RGB{1,2,.5}:RGB{0,0,0}):
                    RGB{x<w/2?1.0:0.0,y<h/3?.5:0.0,x>=w/2?.25:0.0};
            std::vector<float> upload(w*h*4,1);
            for(int y=0;y<h;++y) for(int x=0;x<w;++x) for(int c=0;c<3;++c)
                upload[((h-1-y)*w+x)*4+c]=pixels[y*w+x][c];
            glBindTexture(GL_TEXTURE_2D,crt.input().texture.get());
            glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_FLOAT,upload.data());
            for(float time : {0.0f,8.0f}) {
                crt.draw(output.framebuffer.get(),GL_BACK,time,false);
                std::vector<unsigned char> image(w*h*4);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,image.data());
                for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
                    auto expected=reference(pixels,w,h,x,y,time);
                    const int p=((h-1-y)*w+x)*4;
                    if(image[p+3]!=255) throw std::runtime_error("CRT alpha");
                    for(int c=0;c<3;++c) worst=std::max(worst,std::abs(int(image[p+c])-int(std::lround(std::clamp(expected[c],0.0,1.0)*255))));
                }
            }
        }
    }
    check_gl("CRT reference check");
    if(worst>2) throw std::runtime_error("CRT reference pixel error "+std::to_string(worst));
    std::cout<<"CRT CPU reference: mask/scan coordinates, RGB edges, horizontal bleed, black lift, HDR knee; worst="<<worst<<"\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
