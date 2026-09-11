#pragma once
#include "renderer.h"
#include <memory>

namespace reflow {
void validate_simulation(const MMSettings& settings, float aspect);
class Simulation {
public:
    Simulation(MMSettings settings, float aspect, std::uint64_t seed);
    void update(MMSettings settings, float aspect);
    void advance(double elapsed);
    void clock(int hour, int minute, int second);
    RenderView view() const;
    const MMGlyphInstance* data() const { return instances_.data(); }
    std::size_t count() const { return count_; }
    std::uint64_t steps() const { return steps_; }
    double residual() const { return residual_; }
    double travel() const { return travel_; }
    double dropped_time() const { return dropped_; }
private:
    std::unique_ptr<MMSim, decltype(&mm_sim_destroy)> sim_{nullptr, mm_sim_destroy};
    MMSettings settings_;
    float aspect_;
    std::vector<MMGlyphInstance> instances_;
    std::size_t count_ = 0;
    std::uint64_t steps_ = 0;
    double residual_ = 0, travel_ = 0, dropped_ = 0;
};
}
