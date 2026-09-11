#include "simulation.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
static void check(bool v,const char* m) { if(!v) throw std::runtime_error(m); }
int main() try {
    auto settings=mm_settings_default();
    for(float aspect : {16.0f/9,32.0f/9,9.0f/16}) for(double depth : {0.,1.5}) {
        settings.depthAmount=depth;
        reflow::Simulation a(settings,aspect,1234), b(settings,aspect,1234);
        a.clock(13,0,0); b.clock(13,0,0);
        const int iterations=depth>0 && aspect==16.0f/9 ? 2400:300;
        for(int i=0;i<iterations;++i) { a.advance(1.0/30); for(int j=0;j<4;++j) b.advance(1.0/120); }
        check(a.steps()==static_cast<unsigned>(iterations*2) && b.steps()==a.steps(),"FPS-dependent step count");
        check(a.count()>0 && a.count()==b.count(),"count mismatch");
        check(!std::memcmp(a.data(),b.data(),a.count()*sizeof(MMGlyphInstance)),"FPS-dependent instances");
        check(depth==0 ? a.travel()==0 : std::abs(a.travel()-std::fmod(iterations/30.0*2,72))<1e-7,"camera rebase");
        for(float v : a.view().view_projection) check(std::isfinite(v),"invalid matrix");
        for(std::size_t i=0;i<a.count();++i) {
            const auto& g=a.data()[i]; check(g.cell>=0 && g.cell<57,"glyph bounds");
            check(std::isfinite(g.py) && std::isfinite(g.pz),"nonfinite position");
            if(i) check(g.pz>=a.data()[i-1].pz,"depth order");
        }
        const auto before=a.steps(); a.advance(600); check(a.steps()==before+6,"pause catch-up");
        check(a.dropped_time()>599,"dropped-time accounting");
        auto changed=settings;changed.density=.4;changed.glyphScale=.2f;
        a.update(changed,21.0f/9);a.advance(1.0/60);
    }
    bool failed=false;
    try { auto bad=settings;bad.speed=NAN;reflow::validate_simulation(bad,1); } catch(const std::exception&) { failed=true; }
    check(failed,"NaN accepted"); failed=false;
    try { auto bad=settings;bad.glyphScale=.1f;reflow::validate_simulation(bad,64); } catch(const std::exception&) { failed=true; }
    check(failed,"over-budget settings accepted");
    std::cout << "FPS independence, camera recycling, projection, bounds, pause and update OK\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
