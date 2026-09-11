#include "renderer.h"
#include "glx_context.h"
#include "x11_host.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
static void require(bool b,const char* message) {if(!b) throw std::runtime_error(message);}
int main() try {
    using namespace reflow;
    X11Host host(128,96,false);GlxContext context(host.display(),host.config(),host.window());
    Renderer renderer;
    require(renderer.crt_storage_bytes()==0,"CRT allocated while disabled");
    int worst=0;
    for(int i=0;i<20;++i) {
        int w=i==0?1:17+i*7,h=i==0?1:13+i*5;
        renderer.resize(w,h);
        PostSettings post{true,i%2==0,.9f,.2f,8};
        renderer.postprocess(post);renderer.draw_control();const auto off=renderer.read_rgba();
        post.crt=true;post.crt_identity=true;
        renderer.postprocess(post);renderer.draw_control();const auto on=renderer.read_rgba();
        require(renderer.crt_storage_bytes()==static_cast<std::uint64_t>(w)*h*8,"wrong CRT storage/dimensions");
        require(on==renderer.read_rgba(),"capture compounded the CRT pass");
        for(std::size_t p=0;p<on.size();++p) {
            worst=std::max(worst,std::abs(int(on[p])-off[p]));
            if(p%4==3) require(on[p]==255,"CRT alpha");
        }
        post.crt=false;renderer.postprocess(post);
        require(renderer.crt_storage_bytes()==0,"disabled CRT storage not released");
        renderer.draw_control();require(off==renderer.read_rgba(),"CRT bypass changed original output");
    }
    require(worst<=1,"identity pass changed visible output");
    renderer.postprocess({true,true,.9f,0,8,true,true});renderer.draw_control();
    renderer.resize(0,0);require(renderer.crt_storage_bytes()==0,"zero-size CRT retained");
    check_gl("CRT lifecycle");
    std::cout<<"20 toggles/resizes, identity error <= "<<worst<<", bypass and CRT allocation release OK\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
