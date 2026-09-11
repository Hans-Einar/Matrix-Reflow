#include "mmcore.h"
#include <iostream>

int main() {
    const auto settings = mm_settings_default();
    std::cout << "Matrix Reflow Linux 0.1.0: build foundation\n"
              << "Core default density: " << settings.density << '\n';
}
