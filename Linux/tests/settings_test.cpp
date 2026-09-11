#include "settings.h"
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
using namespace reflow;
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
template<class F> void fails(F fn) {bool failed=false;try{fn();}catch(const std::exception&){failed=true;}check(failed,"Expected rejection");}
std::string bytes(const std::string& path) {std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
int main() try {
    gchar* temp=g_dir_make_tmp("matrix-settings-XXXXXX",nullptr);check(temp,"Temporary directory");
    const std::string dir=temp;g_free(temp);
    struct Cleanup {std::string dir;~Cleanup(){chmod(dir.c_str(),0700);std::filesystem::remove_all(dir);}} cleanup{dir};
    const auto path=dir+"/settings.ini";
    auto p=load_profiles(path);check(p.selected==1,"Missing file defaults");
    p.selected=4;p.profiles[3].name="Grønn ; $(never a shell)";
    for(size_t i=0;i<5;++i) {
        set_setting(p.profiles[i].settings,"speed",setting_number(i*.2));
        set_setting(p.profiles[i].settings,"main-red","0.123456789");
        set_setting(p.profiles[i].settings,"crt","1");
    }
    save_profiles(p,path);auto q=load_profiles(path);
    check(q.selected==4 && q.profiles[3].name==p.profiles[3].name,"Names/selection");
    for(size_t i=0;i<5;++i) check(effective_settings(p.profiles[i].settings)==effective_settings(q.profiles[i].settings),"Exact setting round trip");
    struct stat st{};check(!stat(path.c_str(),&st) && (st.st_mode&0777)==0600,"Private settings file");
    for(const auto& f:setting_fields()) {
        Settings s;set_setting(s,f.key,setting_number(f.low));set_setting(s,f.key,setting_number(f.high));
        for(const auto& bad:{"nan","inf","-inf","","1x"}) fails([&]{set_setting(s,f.key,bad);});
        fails([&]{set_setting(s,f.key,setting_number(f.low-1));});
        fails([&]{set_setting(s,f.key,setting_number(f.high+1));});
        if(f.type!=FieldType::Number) fails([&]{set_setting(s,f.key,"0.5");});
    }
    const auto original=bytes(path);
    auto invalid=p;invalid.profiles[1].settings.fps_limit=0;
    fails([&]{save_profiles(invalid,path);});check(bytes(path)==original,"Invalid save preserves file");
    if(geteuid()!=0) {chmod(dir.c_str(),0500);fails([&]{save_profiles(p,path);});chmod(dir.c_str(),0700);check(bytes(path)==original,"Failed atomic replacement preserves file");}
    {std::ofstream file(path);file<<"[Settings]\nversion=99\nselected=1\n";}
    const auto future=bytes(path);fails([&]{load_profiles(path);});fails([&]{save_profiles(p,path);});
    check(bytes(path)==future,"Future file never overwritten");
    std::cout<<"settings validation, five profiles, exact colors, private atomic storage: OK\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
