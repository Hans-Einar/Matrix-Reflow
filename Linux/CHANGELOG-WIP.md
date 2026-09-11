# Linux — work in progress

## Iteration 1

* Native CMake build for the shared C99 simulation and a C++17 Linux executable.
* Display-independent core test checks valid glyphs, determinism and buffer bounds.

* Embedded Matrix font and FreeType atlas with all 57 original glyphs;
  `--dump-atlas FILE.pgm` works without X11.

XScreenSaver integration, animated rendering, bloom and CRT are not yet implemented.
