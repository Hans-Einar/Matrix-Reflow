#include "preview.h"
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>
#ifndef REFLOW_RENDERER_PATH
#define REFLOW_RENDERER_PATH "/usr/local/bin/matrix-reflow"
#endif
namespace reflow {
Preview::~Preview() { stop(); }
void Preview::start(const std::string& executable,const std::vector<std::string>& arguments) {
    stop();std::vector<std::string> storage{executable};storage.insert(storage.end(),arguments.begin(),arguments.end());
    std::vector<gchar*> argv;for(auto& value:storage) argv.push_back(value.data());argv.push_back(nullptr);
    GError* error=nullptr;
    if(!g_spawn_async(nullptr,argv.data(),nullptr,G_SPAWN_DO_NOT_REAP_CHILD,nullptr,nullptr,&pid_,&error)) {
        const std::string message=error->message;g_error_free(error);pid_=0;throw std::runtime_error(message);
    }
    failed_=false;
}
bool Preview::poll() {
    if(!pid_) return false;
    int status=0;const auto result=waitpid(pid_,&status,WNOHANG);
    if(result==pid_ || (result<0 && errno==ECHILD)) {
        failed_=result<0 || !WIFEXITED(status) || WEXITSTATUS(status)!=0;
        g_spawn_close_pid(pid_);pid_=0;
    }
    return pid_!=0;
}
void Preview::stop() {
    if(!poll()) return;
    kill(pid_,SIGTERM);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(1);
    while(poll() && std::chrono::steady_clock::now()<deadline) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if(pid_) {
        kill(pid_,SIGKILL);int status;
        while(waitpid(pid_,&status,0)<0 && errno==EINTR) {}
        g_spawn_close_pid(pid_);pid_=0;
    }
    failed_=false;
}
std::string renderer_executable() {
    std::error_code error;
    const auto self=std::filesystem::read_symlink("/proc/self/exe",error);
    if(!error) {
        const auto sibling=self.parent_path()/"matrix-reflow";
        if(access(sibling.c_str(),X_OK)==0) return sibling;
    }
    if(access(REFLOW_RENDERER_PATH,X_OK)==0) return REFLOW_RENDERER_PATH;
    gchar* found=g_find_program_in_path("matrix-reflow");
    if(found) {std::string result=found;g_free(found);return result;}
    throw std::runtime_error("Cannot find matrix-reflow beside settings executable or on PATH");
}
}
