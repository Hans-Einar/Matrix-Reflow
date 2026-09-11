#include "preview.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
void check(bool ok,const char* text) {if(!ok) throw std::runtime_error(text);}
int main(int argc,char** argv) try {
    if(argc==4 && std::string(argv[1])=="--fixture") {std::ofstream(argv[2])<<argv[3];return 0;}
    if(argc==3 && std::string(argv[1])=="--stubborn") {
        signal(SIGTERM,SIG_IGN);std::ofstream(argv[2])<<"ready";for(;;) pause();
    }
    const auto dir=std::filesystem::temp_directory_path()/("reflow-preview-"+std::to_string(getpid()));std::filesystem::create_directory(dir);
    struct Cleanup {std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{dir};
    const auto file=(dir/"arguments").string();const auto self=std::filesystem::canonical("/proc/self/exe").string();
    reflow::Preview preview;
    const std::string literal="name with spaces; $(touch NEVER) ' \" `echo no`";
    preview.start(self,{"--fixture",file,literal});
    for(int i=0;i<200 && preview.poll();++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    check(!preview.poll() && !preview.failed(),"Child exit/reaping");
    std::ifstream in(file);const std::string saved{std::istreambuf_iterator<char>(in),{}};check(saved==literal,"Literal argv without shell");
    reflow::Preview unrelated;unrelated.start("/usr/bin/sleep",{"10"});
    preview.start("/usr/bin/sleep",{"10"});preview.stop();check(unrelated.poll(),"Unrelated process survives");
    preview.start(self,{"--stubborn",(dir/"ready").string()});
    for(int i=0;i<200 && !std::filesystem::exists(dir/"ready");++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    check(std::filesystem::exists(dir/"ready"),"Stubborn child ready");
    const auto start=std::chrono::steady_clock::now();preview.stop();
    check(std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<2,"Bounded forced cleanup");
    check(unrelated.poll(),"Only owned child stopped");unrelated.stop();
    bool failed=false;try{preview.start("/no/such/renderer",{});}catch(const std::exception&){failed=true;}
    check(failed && !preview.poll(),"Spawn error handled");
    std::cout<<"Literal argument vector, exit, replacement ownership, bounded cleanup: OK\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
