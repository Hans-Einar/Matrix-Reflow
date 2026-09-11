#include "mmcore.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

int main() try {
    static_assert(sizeof(MMGlyphInstance) == 40);
    const auto settings = mm_settings_default();
    using Sim = std::unique_ptr<MMSim, decltype(&mm_sim_destroy)>;
    Sim a(mm_sim_create(&settings, 57, 12345, 16.0f / 9), mm_sim_destroy);
    Sim b(mm_sim_create(&settings, 57, 12345, 16.0f / 9), mm_sim_destroy);
    const int capacity = mm_sim_max_instances(a.get());
    require(capacity > 0 && capacity == mm_sim_max_instances(b.get()), "capacity");
    std::vector<MMGlyphInstance> x(capacity), y(capacity);
    for (int i = 0; i < 120; ++i) {
        mm_sim_advance(a.get(), 1.0f / 60);
        mm_sim_advance(b.get(), 1.0f / 60);
    }
    const int n = mm_sim_write_instances(a.get(), x.data(), capacity, 1.0f / 120);
    const int m = mm_sim_write_instances(b.get(), y.data(), capacity, 1.0f / 120);
    require(n > 0 && n <= capacity && n == m, "emitted count");
    require(std::memcmp(x.data(), y.data(), n * sizeof(x[0])) == 0, "determinism");
    for (int i = 0; i < n; ++i) {
        float values[10];
        std::memcpy(values, &x[i], sizeof(values));
        for (float v : values) require(std::isfinite(v), "non-finite instance");
        require(x[i].cell >= 0 && x[i].cell < 57, "glyph index");
        require(std::floor(x[i].cell) == x[i].cell, "fractional glyph index");
        require(std::abs(x[i].flipX) == 1 && std::abs(x[i].flipY) == 1, "flip");
        require(x[i].bright >= 0 && x[i].bright <= 0.80001f, "brightness");
    }
    struct { MMGlyphInstance items[3]; unsigned char guard[40]; } small;
    std::memset(&small, 0xA5, sizeof(small));
    require(mm_sim_write_instances(a.get(), small.items, 3, 0) == 3, "small buffer");
    for (unsigned char v : small.guard) require(v == 0xA5, "buffer overwrite");
    std::cout << n << " deterministic, valid instances; capacity " << capacity << '\n';
    return 0;
} catch (const std::exception& e) {
    std::cerr << "core-smoke: " << e.what() << '\n';
    return 1;
}
