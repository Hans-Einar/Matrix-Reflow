#include "font_atlas.h"
#include "glx_context.h"
#include "glyph_lab.h"
#include "renderer.h"
#include "x11_host.h"
#include <charconv>
#include <chrono>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t stopped = 0;
void stop(int) { stopped = 1; }
int number(const std::string& value, int limit) {
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || result < 1 || result > limit)
        throw std::runtime_error("Expected integer 1.." + std::to_string(limit) + ": " + value);
    return result;
}
}
int main(int argc, char** argv) try {
    int width = 1152, height = 640, frames = 0;
    bool visible = true, control = false;
    std::string capture, dump_atlas;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i == argc) throw std::runtime_error("Missing value for " + arg);
            return argv[i];
        };
        if (arg == "--help") {
            std::cout << "Matrix Reflow Linux - iteration 1\n"
                         "  --windowed           Show all 57 glyphs, flips and colors (default)\n"
                         "  --size WIDTHxHEIGHT  Window size, each dimension 1..16384\n"
                         "  --control            Display the renderer control scene\n"
                         "  --frames N           Exit after N frames (default: until closed)\n"
                         "  --capture FILE.ppm   Save one frame (last with --frames, otherwise first)\n"
                         "  --hidden             Leave own X11 window unmapped for tests\n"
                         "  --dump-atlas FILE.pgm Rasterize font and exit; no display needed\n"
                         "  --version            Print version\n"
                         "Escape or window close exits. XScreenSaver support follows in iteration 2.\n";
            return 0;
        } else if (arg == "--version") { std::cout << "Matrix Reflow Linux 0.1.0\n"; return 0; }
        else if (arg == "--size") {
            const auto size = value();
            const auto x = size.find('x');
            if (x == std::string::npos) throw std::runtime_error("Size must be WIDTHxHEIGHT");
            width = number(size.substr(0, x), 16384);
            height = number(size.substr(x + 1), 16384);
        } else if (arg == "--frames") frames = number(value(), 1000000);
        else if (arg == "--capture") capture = value();
        else if (arg == "--dump-atlas") dump_atlas = value();
        else if (arg == "--hidden") visible = false;
        else if (arg == "--control") control = true;
        else if (arg != "--windowed")
            throw std::runtime_error("Unknown argument: " + arg + "; use --help");
    }
    if (!dump_atlas.empty()) { reflow::build_atlas().write_pgm(dump_atlas); return 0; }
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    reflow::X11Host host(width, height, visible);
    reflow::GlxContext context(host.display(), host.config(), host.window());
    std::cout << context.description() << '\n';
    reflow::Renderer renderer; // Dies before context, which dies before host.
    const auto instances = reflow::glyph_lab_instances();
    int rendered = 0;
    while (!stopped && host.poll() && (!frames || rendered < frames)) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(16);
        renderer.resize(host.width(), host.height());
        if (control) renderer.draw_control();
        else renderer.draw_instances(instances.data(), instances.size(), reflow::glyph_lab_view(host.width(), host.height()));
        ++rendered;
        if (!capture.empty() && (frames ? rendered == frames : rendered == 1)) renderer.write_ppm(capture);
        context.present();
        std::this_thread::sleep_until(deadline);
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << "matrix-reflow: " << e.what() << '\n';
    return 1;
}
