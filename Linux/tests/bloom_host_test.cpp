#include "renderer.h"
#include "glyph_lab.h"
#include "glx_context.h"
#include "x11_host.h"
#include "x11_error.h"
#include <iostream>
#include <memory>
#include <stdexcept>
int main() try {
    using namespace reflow;
    const auto instances=glyph_lab_instances();
    for(auto size : {std::pair{1,1},{3,5},{320,180},{681,382}}) {
        X11Host owner(size.first,size.second,false);
        auto draw=[&](bool borrowed,PostSettings post) {
            auto borrower=borrowed?std::make_unique<X11Host>(owner.window()):nullptr;
            auto& host=borrowed?*borrower:owner;
            XErrorTrap errors(host.display());GlxContext context(host.display(),host.config(),host.window());
            Renderer renderer(build_atlas(),context.double_buffered());
            renderer.resize(host.width(),host.height());renderer.postprocess(post);
            renderer.draw_instances(instances.data(),instances.size(),glyph_lab_view(host.width(),host.height()));
            return renderer.read_rgba();
        };
        for(PostSettings post : {PostSettings{true,true,.9f,0,8}, {true,false,.9f,0,8},
                                {true,true,0,0,8}, {true,true,.9f,1,8}})
            if(draw(false,post)!=draw(true,post)) throw std::runtime_error("Owned/borrowed bloom mismatch");
    }
    std::cout<<"Owned and borrowed output match with bloom on/off/zero, distortion and tiny sizes\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
