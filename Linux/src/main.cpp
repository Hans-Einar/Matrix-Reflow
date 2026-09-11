#include "font_atlas.h"
#include "settings.h"
#include "glx_context.h"
#include "glyph_lab.h"
#include "simulation.h"
#include <ctime>
#include "renderer.h"
#include "x11_host.h"
#include "x11_error.h"
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
    bool visible = true, control = false, lab = false, windowed = false, root = false;
    Window window_id=0;
    reflow::Settings effective;
    auto& settings=effective.rain;
    int profile=-1; bool no_config=false,print_settings=false;
    std::vector<std::pair<std::string,std::string>> overrides;
    std::uint64_t seed = 12345;
    double warmup = 0, duration = 0;
    int bloom_level=0;
    bool post_enabled=true, post_explicit=false, snapshot=false, crt_identity=false;
    auto& fps_limit=effective.fps_limit; bool stats=false;
    std::string capture, dump_atlas;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i == argc) throw std::runtime_error("Missing value for " + arg);
            return argv[i];
        };
        if (arg == "--help") {
            std::cout << "Matrix Reflow Linux - iteration 5\n"
                         "  --windowed           Show animated rain (default)\n"
                         "  --root               Render in XSCREENSAVER_WINDOW (never desktop root)\n"
                         "  --window-id ID       Render in a borrowed X11 window\n"
                         "  --size WIDTHxHEIGHT  Window size, each dimension 1..16384\n"
                         "  --glyph-lab          Static font/flip demonstration\n"
                         "  --seed N             Deterministic seed (1..1000000)\n"
                         "  --snapshot-time N    Freeze at N simulated seconds (0..120), fixed clock\n"
                         "  --warmup N           Simulate 0..120 seconds before display\n"
                         "  --crt-identity       Diagnostic identity pass through CRT target\n"
                         "  --no-post            Raw iteration-2 output, for comparisons\n"
                         "  --bloom-level N      Inspect extracted bloom level 1..5\n"
                         "  --control            Display the renderer control scene\n"
                         "  --duration N         Stop after N wall-clock seconds\n"
                         "  --stats              Report interval FPS and frame work time\n"
                         "  --frames N           Exit after N frames (default: until closed)\n"
                         "  --capture FILE.ppm   Save one frame (last with --frames, otherwise first)\n"
                         "  --hidden             Leave own X11 window unmapped for tests\n"
                         "  --dump-atlas FILE.pgm Rasterize font and exit; no display needed\n"
                         "  --version            Print version\n"
                         "Escape closes owned windows. The host controls borrowed windows.\n";
            std::cout << "\nProfiles (defaults -> saved profile -> explicit switches):\n"
                         "  --profile 0..5       0: defaults; 1..5: saved slot (default: active slot)\n"
                         "  --no-config          Use built-in defaults without reading a file\n"
                         "  --print-effective-settings  Print resolved values without opening X11\n"
                         "Settings file: " << reflow::settings_path() << "\n\nSettings:\n";
            const reflow::Settings defaults;
            for(const auto& f:reflow::setting_fields()) {
                std::cout<<"  --"<<f.key;
                if(f.type==reflow::FieldType::Boolean) std::cout<<" / --no-"<<f.key;
                else std::cout<<" N";
                std::cout<<"  "<<f.label<<" ["<<reflow::setting_number(f.low)<<".."<<reflow::setting_number(f.high)
                         <<"] (default "<<reflow::setting_number(f.get(defaults))<<")\n";
            }
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
        else if (arg == "--no-config") no_config=true;
        else if (arg == "--profile") { const auto v=value();profile=v=="0"?0:number(v,5); }
        else if (arg == "--print-effective-settings") print_settings=true;
        else if (arg.rfind("--",0)==0 && reflow::setting_field(arg.substr(arg.rfind("--no-",0)==0?5:2))) {
            const bool negative=arg.rfind("--no-",0)==0;
            const auto key=arg.substr(negative?5:2);const auto* field=reflow::setting_field(key);
            if(negative && field->type!=reflow::FieldType::Boolean) throw std::runtime_error("Invalid switch: "+arg);
            const auto v=field->type==reflow::FieldType::Boolean?(negative?"0":"1"):value();
            reflow::Settings check;reflow::set_setting(check,key,v);
            overrides.emplace_back(key,v);
            if(key=="crt") crt_identity=false;
            if(key=="bloom" || key=="crt" || key=="bloom-strength" || key=="distortion") post_explicit=true;
        }
        else if (arg == "--crt-identity") {overrides.emplace_back("crt","1");crt_identity=true;post_explicit=true;}
        else if (arg == "--no-post") post_enabled=false;
        else if (arg == "--bloom-level") bloom_level=number(value(),5);
        else if (arg == "--stats") stats=true;
        else if (arg == "--control") control = true;
        else if (arg == "--glyph-lab") lab = true;
        else if (arg == "--seed") seed = static_cast<std::uint64_t>(number(value(),1000000));
        else if (arg == "--warmup" || arg == "--duration" || arg == "--snapshot-time") {
            const auto text=value();double v=0;
            const auto parsed=std::from_chars(text.data(),text.data()+text.size(),v);
            if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size() || !std::isfinite(v))
                throw std::runtime_error("Invalid number for "+arg);
            if(arg=="--duration") {if(v<=0 || v>86400) throw std::runtime_error("Duration must be 0..86400 seconds");duration=v;}
            else {if(v<0 || v>120) throw std::runtime_error("Warmup/snapshot time must be 0..120");warmup=v;snapshot=arg=="--snapshot-time";}
        }
        else if (arg == "--windowed") windowed=true;
        else if (arg == "--root") root=true;
        else if (arg == "--window-id") {
            auto id=value();const bool hex=id.rfind("0x",0)==0;if(hex) id.erase(0,2);
            const auto parsed=std::from_chars(id.data(),id.data()+id.size(),window_id,hex?16:10);
            if(parsed.ec!=std::errc{} || parsed.ptr!=id.data()+id.size() || !window_id || window_id>0xffffffffUL)
                throw std::runtime_error("Invalid X11 host window ID");
        }
        else
            throw std::runtime_error("Unknown argument: " + arg + "; use --help");
    }
    if(no_config && profile>0) throw std::runtime_error("--no-config conflicts with --profile 1..5");
    std::string source="defaults";
    if(!no_config && profile!=0) {
        const auto profiles=reflow::load_profiles(reflow::settings_path());
        const int slot=profile<0?profiles.selected:profile;
        effective=profiles.profiles[slot-1].settings;
        source="profile "+std::to_string(slot)+" ("+profiles.profiles[slot-1].name+") from "+reflow::settings_path()+" (defaults if absent)";
    }
    for(const auto& [key,value]:overrides) reflow::set_setting(effective,key,value);
    reflow::validate_settings(effective);
    if(print_settings) {
        std::cout<<"# Base: "<<source<<"\n# Explicit overrides:";
        for(const auto& [key,value]:overrides) std::cout<<" "<<key<<"="<<value;
        std::cout<<"\n"<<reflow::effective_settings(effective);return 0;
    }
    if (!dump_atlas.empty()) { reflow::build_atlas().write_pgm(dump_atlas); return 0; }
    if(windowed && (root || window_id)) throw std::runtime_error("--windowed conflicts with host-window options");
    if(!windowed && !window_id) {
        if(const char* env=std::getenv("XSCREENSAVER_WINDOW")) {
            std::string id=env;const bool hex=id.rfind("0x",0)==0;if(hex) id.erase(0,2);
            const auto parsed=std::from_chars(id.data(),id.data()+id.size(),window_id,hex?16:10);
            if(parsed.ec!=std::errc{} || parsed.ptr!=id.data()+id.size() || !window_id || window_id>0xffffffffUL)
                throw std::runtime_error("Invalid XSCREENSAVER_WINDOW");
        }
    }
    if(root && !window_id) throw std::runtime_error("--root requires a host window; desktop root is never used");
    if(window_id && !visible) throw std::runtime_error("--hidden only applies to owned windows");
    reflow::validate_simulation(settings,static_cast<float>(width)/height);
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    auto host_ptr=window_id ? std::make_unique<reflow::X11Host>(window_id) : std::make_unique<reflow::X11Host>(width,height,visible);
    auto& host=*host_ptr;
    // Drivers can query the drawable during drawing and resource destruction,
    // not just swap. Keep this alive until all GL objects have been released.
    reflow::XErrorTrap graphics_errors(host.display());
    reflow::GlxContext context(host.display(), host.config(), host.window());
    std::cout << context.description() << std::endl;
    reflow::Renderer renderer(reflow::build_atlas(),context.double_buffered()); // Dies before context, which dies before host.
    renderer.bloom_level(bloom_level);
    const auto instances = reflow::glyph_lab_instances();
    reflow::Simulation simulation(settings,static_cast<float>(host.width())/host.height(),seed);
    if(snapshot) simulation.clock(12,0,0);
    for(int i=0;i<static_cast<int>(warmup*60);++i) simulation.advance(1.0/60);
    int last_width=host.width(),last_height=host.height();
    auto previous=std::chrono::steady_clock::now();
    const auto started=previous;auto interval_started=previous;
    int rendered = 0,interval_frames=0;double interval_work=0,max_work=0;
    while (!stopped && host.poll() && (!frames || rendered < frames)) {
        const auto frame_start=std::chrono::steady_clock::now();
        if(duration>0 && std::chrono::duration<double>(frame_start-started).count()>=duration) break;
        const auto deadline=frame_start+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0/fps_limit));
        if(host.width()<1 || host.height()<1) { std::this_thread::sleep_until(deadline);continue; }
        const auto now=std::chrono::steady_clock::now();
        const double elapsed=std::chrono::duration<double>(now-previous).count();previous=now;
        if(last_width!=host.width() || last_height!=host.height()) {
            simulation.update(settings,static_cast<float>(host.width())/host.height());
            last_width=host.width();last_height=host.height();
        }
        const auto wall=std::time(nullptr);std::tm local{};
        if(!snapshot && localtime_r(&wall,&local)) simulation.clock(local.tm_hour,local.tm_min,local.tm_sec);
        if(!snapshot) simulation.advance(elapsed);
        renderer.resize(host.width(), host.height());
        renderer.postprocess({post_enabled && (!(control||lab) || post_explicit),settings.bloom!=0,
            static_cast<float>(settings.bloomIntensity),static_cast<float>(settings.crtDistort),simulation.view().time,settings.crtEmulation!=0,crt_identity});
        if (control) renderer.draw_control();
        else if(lab) renderer.draw_instances(instances.data(), instances.size(), reflow::glyph_lab_view(host.width(), host.height()));
        else renderer.draw_instances(simulation.data(), simulation.count(), simulation.view());
        ++rendered;
        if (!capture.empty() && (frames ? rendered == frames : rendered == 1)) renderer.write_ppm(capture);
        if(!context.present() || graphics_errors.error()) break;
        const auto ended=std::chrono::steady_clock::now();
        const double work=std::chrono::duration<double,std::milli>(ended-frame_start).count();
        ++interval_frames;interval_work+=work;max_work=std::max(max_work,work);
        const double interval=std::chrono::duration<double>(ended-interval_started).count();
        if(stats && interval>=5) {
            std::cout<<"stats seconds="<<std::chrono::duration<double>(ended-started).count()
                     <<" frames="<<rendered<<" fps="<<interval_frames/interval
                     <<" mean_work_ms="<<interval_work/interval_frames<<" max_work_ms="<<max_work
                     <<" instances="<<simulation.count()<<" dropped_seconds="<<simulation.dropped_time()<<std::endl;
            interval_started=ended;interval_frames=0;interval_work=0;max_work=0;
        }
        std::this_thread::sleep_until(deadline);
    }
    if(stats) std::cout<<"completed frames="<<rendered<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<std::endl;
    return 0;
} catch (const std::exception& e) {
    std::cerr << "matrix-reflow: " << e.what() << '\n';
    return 1;
}
