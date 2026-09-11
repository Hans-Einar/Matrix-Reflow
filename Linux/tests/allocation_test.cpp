#include "mmcore.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
static int remaining=-1;
static bool fail() { if(remaining<0) return false; if(remaining--==0) {remaining=-1;return true;} return false; }
extern "C" void* __real_malloc(std::size_t);
extern "C" void* __real_calloc(std::size_t,std::size_t);
extern "C" void* __wrap_malloc(std::size_t n) { return fail()?nullptr:__real_malloc(n); }
extern "C" void* __wrap_calloc(std::size_t n,std::size_t s) { return fail()?nullptr:__real_calloc(n,s); }
static void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
int main() try {
    auto s=mm_settings_default();
    for(int failure=0;failure<9;++failure) {
        remaining=failure;
        auto* sim=mm_sim_create(&s,57,12,16.0f/9);remaining=-1;
        check(!sim,"creation did not report allocation failure");
    }
    auto* sim=mm_sim_create(&s,57,12,16.0f/9);check(sim,"initial allocation");
    for(int i=0;i<180;++i) mm_sim_advance(sim,1.0f/60);
    const int cap=mm_sim_max_instances(sim);
    std::vector<MMGlyphInstance> before(cap),after(cap);
    const int count=mm_sim_write_instances(sim,before.data(),cap,0);
    auto larger=s;larger.glyphScale=.2f;larger.depthAmount=.5;
    for(int failure=0;failure<8;++failure) {
        remaining=failure;
        const int ok=mm_sim_update_checked(sim,&larger,57,16.0f/9);remaining=-1;
        check(!ok,"growth did not report allocation failure");
        check(mm_sim_max_instances(sim)==cap,"failed growth changed capacity");
        check(mm_sim_write_instances(sim,after.data(),cap,0)==count,"failed growth changed count");
        check(!std::memcmp(before.data(),after.data(),count*sizeof(before[0])),"failed growth changed state");
    }
    check(mm_sim_update_checked(sim,&larger,57,16.0f/9),"valid growth failed");
    mm_sim_destroy(sim);
    std::cout<<"9 creation and 8 growth allocation failures handled; old state retained\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
