#include "simulation.h"
#include "glyph_table.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace reflow {
namespace {
constexpr double step = 1.0 / 60;
void range(double v, double lo, double hi, const char* name) {
    if (!std::isfinite(v) || v < lo || v > hi)
        throw std::runtime_error(std::string(name) + " must be in " + std::to_string(lo) + ".." + std::to_string(hi));
}
using Vec = std::array<float,3>;
Vec sub(Vec a, Vec b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
float dot(Vec a, Vec b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vec normalized(Vec a) { const float n=std::sqrt(dot(a,a)); return {a[0]/n,a[1]/n,a[2]/n}; }
Vec cross(Vec a, Vec b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
}
void validate_simulation(const MMSettings& s, float aspect) {
    range(s.density,.05,1,"density"); range(s.speed,0,1,"speed");
    range(s.glyphScale,.1,2,"glyph scale"); range(s.depthAmount,0,1.5,"depth");
    range(s.lengthBias,0,1,"length bias"); range(s.cameraSpeed,0,1,"camera speed");
    range(s.mutationRate,0,1,"mutation rate"); range(aspect,0.00001,64,"aspect");
    for (float c : {s.mainColorR,s.mainColorG,s.mainColorB,s.glitchColorR,s.glitchColorG,s.glitchColorB})
        range(c,0,1,"color");
    const auto world = mm_world(&s);
    const double lanes = std::ceil(mm_strip_count(&s) * std::max(1.0, static_cast<double>(aspect)/MM_REFERENCE_ASPECT));
    if (world.slotCount < 1 || lanes < 1 || lanes * 2 * world.slotCount > MM_MAX_INSTANCES)
        throw std::runtime_error("Simulation exceeds the 1,048,576-instance memory budget");
}
Simulation::Simulation(MMSettings settings, float aspect, std::uint64_t seed)
    : settings_(settings), aspect_(aspect) {
    validate_simulation(settings, aspect);
    sim_.reset(mm_sim_create(&settings, static_cast<int>(glyphs.size()), seed, aspect));
    if (!sim_) throw std::runtime_error("Cannot allocate rain simulation");
    instances_.resize(mm_sim_max_instances(sim_.get()));
}
void Simulation::update(MMSettings settings, float aspect) {
    validate_simulation(settings, aspect);
    if (!mm_sim_update_checked(sim_.get(), &settings, static_cast<int>(glyphs.size()), aspect))
        throw std::runtime_error("Cannot resize rain simulation; old state preserved");
    settings_ = settings; aspect_ = aspect;
    instances_.resize(mm_sim_max_instances(sim_.get()));
}
void Simulation::clock(int h,int m,int s) { mm_sim_set_clock(sim_.get(),h,m,s); }
void Simulation::advance(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0) throw std::runtime_error("Invalid elapsed time");
    const double accumulated = residual_ + elapsed;
    residual_ = std::min(.1, accumulated);
    dropped_ += accumulated - residual_;
    while (residual_ + 1e-12 >= step) {
        if (settings_.depthAmount > 0) {
            travel_ += settings_.cameraSpeed * 2 * step;
            if (travel_ >= 72) { travel_ -= 72; mm_sim_rebase_depth(sim_.get(),72); }
        } else travel_ = 0;
        mm_sim_set_camera_travel(sim_.get(),static_cast<float>(travel_));
        mm_sim_advance(sim_.get(),static_cast<float>(step));
        ++steps_;
        residual_ = std::max(0.0,residual_-step);
    }
    count_ = static_cast<std::size_t>(mm_sim_write_instances(sim_.get(),instances_.data(),
        static_cast<int>(instances_.size()),static_cast<float>(residual_)));
}
RenderView Simulation::view() const {
    RenderView result;
    const double time = steps_ * step + residual_;
    Vec eye{0,0,48}, center{0,0,-8};
    if (settings_.panning) {
        static const Vec eyes[]={{0,3,48},{15,7,45},{0,15,43},{-15,6,45},{0,-5,50},{9,2,38}};
        static const Vec centers[]={{0,0,-8},{-3,0,-8},{0,-3,-10},{3,0,-8},{0,4,-6},{-4,0,-10}};
        const double cycle=time/9;
        const int i=static_cast<int>(std::fmod(std::floor(cycle),6)), j=(i+1)%6;
        const float t=static_cast<float>(std::min(1.0,(cycle-std::floor(cycle))/.45));
        const float e=t*t*(3-2*t);
        for(int k=0;k<3;++k) { eye[k]=eyes[i][k]+e*(eyes[j][k]-eyes[i][k]); center[k]=centers[i][k]+e*(centers[j][k]-centers[i][k]); }
    }
    const float travel=static_cast<float>(travel_+(settings_.depthAmount>0 ? settings_.cameraSpeed*2*residual_:0));
    eye[2]-=travel; center[2]-=travel;
    const Vec forward=normalized(sub(center,eye));
    const Vec right=normalized(cross(forward,{0,1,0}));
    const Vec up=cross(right,forward);
    const std::array<float,16> view={right[0],up[0],-forward[0],0,
        right[1],up[1],-forward[1],0, right[2],up[2],-forward[2],0,
        -dot(right,eye),-dot(up,eye),dot(forward,eye),1};
    const float f=1/std::tan(46.0f*3.14159265358979323846f/360);
    const std::array<float,16> projection={f/aspect_,0,0,0, 0,f,0,0, 0,0,-241.0f/239,-1, 0,0,-480.0f/239,0};
    result.view_projection.fill(0);
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) for(int k=0;k<4;++k)
        result.view_projection[c*4+r]+=projection[k*4+r]*view[c*4+k];
    result.camera_position=eye; result.camera_right=right; result.camera_up=up;
    result.glyph_half=settings_.glyphScale; result.time=static_cast<float>(time);
    result.fog={settings_.fog ? 1.0f:0.0f,34,186};
    result.textured=settings_.textured; result.wireframe=settings_.wireframe;
    result.extra_contrast_heads=settings_.extraContrastHeads;
    return result;
}
}
