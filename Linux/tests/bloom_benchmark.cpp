// Opt-in, bounded render-only measurements. Not part of the default CTest suite.
#include "simulation.h"
#include "glx_context.h"
#include "x11_host.h"
#include "x11_error.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <time.h>
#include <vector>
static double cpu_seconds() {timespec t{};clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char** argv) try {
    using namespace reflow;
    if(argc!=2) throw std::runtime_error("Usage: bloom-benchmark OUTPUT-DIRECTORY");
    std::filesystem::create_directories(argv[1]);
    X11Host probe(16,16,false);
    const int screen=DefaultScreen(probe.display());
    const std::pair actual{DisplayWidth(probe.display(),screen),DisplayHeight(probe.display(),screen)};
    std::cout<<"Actual display "<<actual.first<<'x'<<actual.second<<'\n';
    std::vector<std::pair<int,int>> sizes{{1920,1080}};
    if(actual!=sizes[0]) sizes.push_back(actual);
    for(auto [w,h]:sizes) {
        X11Host host(w,h,false);XErrorTrap errors(host.display());
        GlxContext context(host.display(),host.config(),host.window());
        std::cout<<context.description()<<'\n';
        Renderer renderer;renderer.resize(w,h);
        // Unmapped/obscured default drawables may discard composition fragments.
        // Force full-frame composition into a persistent RGBA8 output instead.
        Bloom::Level output(w,h);
        glBindTexture(GL_TEXTURE_2D,output.texture.get());
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        Simulation simulation(mm_settings_default(),static_cast<float>(w)/h,12345);
        simulation.clock(12,0,0);
        for(int i=0;i<480;++i) simulation.advance(1.0/60);
        GLint bits=0;glGetQueryiv(GL_TIME_ELAPSED,GL_QUERY_COUNTER_BITS,&bits);
        if(!bits) throw std::runtime_error("GPU elapsed-time query unavailable");
        GLuint query;glGenQueries(1,&query);
        for(int mode=0;mode<5;++mode) {
            renderer.postprocess({mode!=0,mode==2 || mode==4,.9f,0,8,mode>=3});
            auto draw=[&] {renderer.draw_instances(simulation.data(),simulation.count(),simulation.view(),output.framebuffer.get());};
            for(int i=0;i<3;++i) draw();glFinish();
            const double cpu_start=cpu_seconds();double submit=0,gpu_sum=0,gpu_max=0;
            for(int i=0;i<12;++i) {
                glBeginQuery(GL_TIME_ELAPSED,query);
                const auto start=std::chrono::steady_clock::now();draw();
                submit+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                glEndQuery(GL_TIME_ELAPSED);
                GLuint64 ns=0;glGetQueryObjectui64v(query,GL_QUERY_RESULT,&ns);
                const double ms=ns/1e6;gpu_sum+=ms;gpu_max=std::max(gpu_max,ms);
            }
            const double cpu_ms=(cpu_seconds()-cpu_start)*1000/12;
            const char* name=mode==0?"raw":mode==1?"no-bloom":mode==2?"bloom":mode==3?"crt":"crt-bloom";
            std::cout<<w<<'x'<<h<<" mode="<<name<<" frames=12 instances="<<simulation.count()
                     <<" gpu_mean_ms="<<gpu_sum/12<<" gpu_max_ms="<<gpu_max
                     <<" crt_bytes="<<renderer.crt_storage_bytes()<<" submit_wall_ms="<<submit/12<<" process_cpu_ms="<<cpu_ms<<std::endl;
            renderer.write_ppm(std::string(argv[1])+"/"+std::to_string(w)+"x"+std::to_string(h)+"-"+name+".ppm");
        }
        glDeleteQueries(1,&query);check_gl("bloom benchmark");
    }
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
