#include "x11_host.h"
#include "glx_context.h"
#include "renderer.h"
#include "x11_error.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
static void check(bool v,const char* s) {if(!v) throw std::runtime_error(s);}
int main() try {
    using namespace reflow;
    X11Host owner(400,240,false);
    XStoreName(owner.display(),owner.window(),"host sentinel");XSync(owner.display(),False);
    {
        X11Host host(owner.window());
        check(!host.owns_window(),"borrowed ownership");
        GlxContext context(host.display(),host.config(),host.window());
        Renderer renderer(build_atlas(),context.double_buffered());
        for(int i=0;i<12;++i) {
            int w=200+i*17,h=100+i*13;owner.resize(w,h);
            for(int n=0;n<100;++n) {
                check(host.poll(),"host vanished");
                if(host.width()==w && host.height()==h) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            check(host.width()==w && host.height()==h,"borrowed resize");
            renderer.postprocess({true,i%2==0,.9f,.5f,8,i%3==0});
            renderer.resize(w,h);renderer.draw_control();check(context.present(),"borrowed presentation");
        }
        bool failed=false;try {host.resize(50,50);} catch(const std::runtime_error&) {failed=true;}
        check(failed,"borrower resized owner's window");
    }
    XWindowAttributes attrs{};
    check(XGetWindowAttributes(owner.display(),owner.window(),&attrs),"borrower destroyed window");
    char* name=nullptr;XFetchName(owner.display(),owner.window(),&name);
    check(name && std::string(name)=="host sentinel","borrower changed title");XFree(name);
    {
        X11Host host(owner.window());
        XErrorTrap lifetime(host.display());
        GlxContext context(host.display(),host.config(),host.window());
        Renderer renderer;renderer.postprocess({true,true,.9f,.5f,8,true});
        renderer.resize(host.width(),host.height());renderer.draw_control();
        XDestroyWindow(owner.display(),owner.window());XSync(owner.display(),False);owner.poll();
        // Destruction can race a frame after poll: Mesa may query geometry
        // during drawing, before the error guard inside present() is reached.
        renderer.draw_control();
        bool alive=true;
        for(int n=0;n<100 && alive;++n) {alive=host.poll();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        check(!alive,"host destruction not noticed");
        check(!context.present(),"destroyed drawable did not report failure");
    }
    bool invalid=false;try {X11Host bad(static_cast<Window>(0xffffffffUL));} catch(const std::exception&) {invalid=true;}
    check(invalid,"invalid window accepted");
    bool root=false;try {X11Host bad(DefaultRootWindow(owner.display()));} catch(const std::exception&) {root=true;}
    check(root,"desktop root accepted");
    const Window input=XCreateWindow(owner.display(),DefaultRootWindow(owner.display()),0,0,10,10,0,0,InputOnly,CopyFromParent,0,nullptr);
    XSync(owner.display(),False);
    bool input_only=false;try {X11Host bad(input);} catch(const std::exception&) {input_only=true;}
    XDestroyWindow(owner.display(),input);check(input_only,"InputOnly window accepted");
    std::cout<<"Borrowed resize/presentation, preserved ownership/title, vanished drawable and invalid windows OK\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
