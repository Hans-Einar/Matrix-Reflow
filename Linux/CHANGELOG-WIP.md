# Linux — work in progress

## Iteration 1

* Native CMake build for the shared C99 simulation and a C++17 Linux executable.
* Display-independent core test checks valid glyphs, determinism and buffer bounds.
* Embedded Matrix font and FreeType atlas with all 57 original glyphs;
  `--dump-atlas FILE.pgm` works without X11.
* Shared OpenGL 3.3/GLX renderer with RGBA16F scene target, opaque composition,
  resize handling, shader diagnostics and controlled shutdown.
* Windowed glyph lab displays all characters, four flip modes, colors and
  brightness using the permanent instanced glyph-rendering path.
* Optional GPU tests validate atlas selection, orientation, color and opaque output.
* Embedded assets allow execution outside the source/build directory; bounded
  frame runs and PPM capture support inspection without mouse automation.

XScreenSaver integration, animated rendering, bloom and CRT are not yet implemented.

## Iteration 2

* Animated rain uses the existing GLX glyph renderer, a fixed 60 Hz simulation,
  bounded catch-up, wall clock, depth, camera path and synchronized recycling.
* Parameter validation and transactional core allocation prevent uncontrolled
  growth and preserve existing state on allocation failure.
* Density changes refresh column spacing as well as column count.
