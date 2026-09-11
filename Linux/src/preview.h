#pragma once
#include <glib.h>
#include <string>
#include <vector>
namespace reflow {
// Holds an unreaped child PID until waitpid succeeds, preventing PID reuse.
class Preview {
public:
    ~Preview();
    Preview() = default;
    Preview(const Preview&) = delete;
    Preview& operator=(const Preview&) = delete;
    void start(const std::string& executable,const std::vector<std::string>& arguments);
    bool poll(); // true while running; reaps completed child
    void stop(); // TERM, bounded grace period, then KILL and reap
    bool failed() const { return failed_; }
private:
    GPid pid_=0;
    bool failed_=false;
};
std::string renderer_executable();
}
