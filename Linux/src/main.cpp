#include "font_atlas.h"
#include "glx_context.h"
#include "glyph_lab.h"
#include "simulation.h"
#include <ctime>
#include "renderer.h"
#include "x11_host.h"
#include <charconv>
#include <chrono>
#include <cmath>
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
    bool visible = true, control = false, lab = false;
    auto settings = mm_settings_default();
    std::uint64_t seed = 12345;
    double warmup = 0;
    std::string capture, dump_atlas;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i == argc) throw std::runtime_error("Missing value for " + arg);
            return argv[i];
        };
        if (arg == "--help") {
            std::cout << "Matrix Reflow Linux - iteration 2\n"
                         "  --windowed           Show animated rain (default)\n"
                         "  --size WIDTHxHEIGHT  Window size, each dimension 1..16384\n"
                         "  --glyph-lab          Static font/flip demonstration\n"
                         "  --speed N            Rain speed 0..1 (default .35)\n"
                         "  --density N          Density .05..1 (default .9)\n"
                         "  --scale N            Glyph scale .1..2 (default .3)\n"
                         "  --depth N            Depth 0..1.5 (default 0)\n"
                         "  --camera-speed N     Camera speed 0..1\n"
                         "  --length N           Length bias 0..1\n"
                         "  --mutation N         Mutation rate 0..1\n"
                         "  --panning            Enable camera path\n"
                         "  --binary             Use 0/1 characters\n"
                         "  --seed N             Deterministic seed (1..1000000)\n"
                         "  --warmup N           Simulate 0..120 seconds before display\n"
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
        else if (arg == "--glyph-lab") lab = true;
        else if (arg == "--seed") seed = static_cast<std::uint64_t>(number(value(),1000000));
        else if (arg == "--panning") settings.panning = 1;
        else if (arg == "--binary") settings.binaryMode = 1;
        else if (arg == "--speed" || arg == "--density" || arg == "--scale" || arg == "--depth" ||
                 arg == "--camera-speed" || arg == "--length" || arg == "--mutation" || arg == "--warmup") {
            const auto text=value(); double v=0;
            const auto parsed=std::from_chars(text.data(),text.data()+text.size(),v);
            if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size() || !std::isfinite(v))
                throw std::runtime_error("Invalid number for " + arg);
            if(arg=="--speed") settings.speed=v;
            else if(arg=="--density") settings.density=v;
            else if(arg=="--scale") settings.glyphScale=static_cast<float>(v);
            else if(arg=="--depth") settings.depthAmount=v;
            else if(arg=="--camera-speed") settings.cameraSpeed=v;
            else if(arg=="--length") settings.lengthBias=v;
            else if(arg=="--mutation") settings.mutationRate=v;
            else { if(v<0 || v>120) throw std::runtime_error("Warmup must be 0..120"); warmup=v; }
        }
        else if (arg != "--windowed")
            throw std::runtime_error("Unknown argument: " + arg + "; use --help");
    }
    if (!dump_atlas.empty()) { reflow::build_atlas().write_pgm(dump_atlas); return 0; }
    reflow::validate_simulation(settings,static_cast<float>(width)/height);
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    reflow::X11Host host(width, height, visible);
    reflow::GlxContext context(host.display(), host.config(), host.window());
    std::cout << context.description() << '\n';
    reflow::Renderer renderer; // Dies before context, which dies before host.
    const auto instances = reflow::glyph_lab_instances();
    reflow::Simulation simulation(settings,static_cast<float>(host.width())/host.height(),seed);
    for(int i=0;i<static_cast<int>(warmup*60);++i) simulation.advance(1.0/60);
    int last_width=host.width(),last_height=host.height();
    auto previous=std::chrono::steady_clock::now();
    int rendered = 0;
    while (!stopped && host.poll() && (!frames || rendered < frames)) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(16);
        const auto now=std::chrono::steady_clock::now();
        const double elapsed=std::chrono::duration<double>(now-previous).count();previous=now;
        if(last_width!=host.width() || last_height!=host.height()) {
            simulation.update(settings,static_cast<float>(host.width())/host.height());
            last_width=host.width();last_height=host.height();
        }
        const auto wall=std::time(nullptr);std::tm local{};
        if(localtime_r(&wall,&local)) simulation.clock(local.tm_hour,local.tm_min,local.tm_sec);
        simulation.advance(elapsed);
        renderer.resize(host.width(), host.height());
        if (control) renderer.draw_control();
        else if(lab) renderer.draw_instances(instances.data(), instances.size(), reflow::glyph_lab_view(host.width(), host.height()));
        else renderer.draw_instances(simulation.data(), simulation.count(), simulation.view());
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
