#include "glx_context.h"
#include "renderer.h"
#include "x11_host.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main() try {
    using namespace reflow;
    X11Host host(320, 240, false);
    GlxContext context(host.display(), host.config(), host.window());
    std::cout << context.description() << '\n';
    {
        bool rejected = false;
        try { Program broken("#version 330 core\nnot valid GLSL", "", "deliberate-test"); }
        catch (const std::runtime_error& e) {
            rejected = std::string(e.what()).find("deliberate-test vertex shader") != std::string::npos;
        }
        require(rejected, "shader compilation failure must identify source/stage");
    }
    Renderer renderer;
    for (int i = 0; i < 30; ++i) {
        const int w = 32 + (i % 5) * 97, h = 24 + (i % 7) * 41;
        host.resize(w, h);
        for (int wait = 0; wait < 100; ++wait) {
            require(host.poll(), "window closed unexpectedly");
            if (host.width() == w && host.height() == h) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(host.width() == w && host.height() == h, "resize event mismatch");
        renderer.resize(w, h);
        renderer.draw_control();
        const auto pixels = renderer.read_rgba();
        require(pixels.size() == static_cast<std::size_t>(w * h * 4), "readback size");
        for (std::size_t p = 3; p < pixels.size(); p += 4) require(pixels[p] == 255, "output alpha");
        require(pixels[0] < 10 && pixels[1] > 240, "top-left orientation/color");
        const auto bottom_right = pixels.size() - 4;
        require(pixels[bottom_right] > 240 && pixels[bottom_right+1] < 10, "bottom-right color");
        require(pixels[2] >= 62 && pixels[2] <= 65, "scene/composite transfer");
        context.present();
    }
    renderer.resize(0, 0);
    renderer.draw_control();
    renderer.resize(32, 24);
    renderer.draw_control();
    XEvent close{};
    close.xclient.type = ClientMessage;
    close.xclient.window = host.window();
    close.xclient.message_type = XInternAtom(host.display(), "WM_PROTOCOLS", False);
    close.xclient.format = 32;
    close.xclient.data.l[0] = static_cast<long>(XInternAtom(host.display(), "WM_DELETE_WINDOW", False));
    XSendEvent(host.display(), host.window(), False, NoEventMask, &close);
    XSync(host.display(), False);
    require(!host.poll(), "WM_DELETE_WINDOW ignored");
    check_gl("graphics-test completion");
    std::cout << "30 resizes, pixel orientation/color/alpha, zero size, shader error and window close OK\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "graphics-test: " << e.what() << '\n';
    return 1;
}
