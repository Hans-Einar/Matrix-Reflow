# Matrix Reflow on Linux

Iteration 1 builds a native C99/C++17 application. Rendering and the font atlas
are being added on this branch; XScreenSaver integration belongs to iteration 2.
See [implementation plan](implementation-plan.md) and [study](study.md).

## Build

The following development packages were verified installed on AlmaLinux 10.2:
`gcc gcc-c++ cmake make pkgconf-pkg-config libX11-devel libepoxy-devel freetype-devel`.
OpenGL 3.3 via GLX is the rendering baseline. No Windows SDK is required.

From the repository root:

```sh
cmake -S Linux -B build/linux -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux --parallel
ctest --test-dir build/linux --output-on-failure
./build/linux/matrix-reflow
```

Use a separate build directory with `-DCMAKE_BUILD_TYPE=Release` for optimized
builds. CPU tests require no display. Missing development dependencies fail at
configure time. Build artifacts stay outside source control.
