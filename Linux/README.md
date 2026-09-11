# Matrix Reflow on Linux

The Linux port now renders animated Matrix rain in its own window and inside
XScreenSaver's actual preview and saver windows. Both use the same OpenGL 3.3/GLX
renderer, embedded FreeType font atlas and shared C simulation. Bloom, CRT and
the full configuration tool follow in iterations 3–5.
See the [implementation plan](implementation-plan.md) and [study](study.md).

## Build and run

Verified on AlmaLinux 10.2 with `gcc gcc-c++ cmake make pkgconf-pkg-config
libX11-devel libepoxy-devel freetype-devel` and Python 3 for registration tests.
No Windows SDK is required.

```sh
cmake -S Linux -B build/linux -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --parallel 2
ctest --test-dir build/linux --output-on-failure
./build/linux/matrix-reflow --windowed
```

Escape or window close exits an owned window. `--help` lists simulation and
diagnostic options. Example: `--depth .7 --panning --speed .4`. `--glyph-lab`
shows all 57 glyphs and flip modes; `--control` shows the diagnostic scene.
Assets are embedded, so execution does not depend on the working directory.
`--dump-atlas /tmp/atlas.pgm` needs no display.

## XScreenSaver installation

For the locally built XScreenSaver under `/usr/local`:

```sh
sudo cmake --install build/linux --prefix /usr/local
python3 Linux/xscreensaver/register.py
xscreensaver-settings
```

Select **Matrix Reflow** to see the embedded preview. Installation places the
executable in both `bin` and `libexec/xscreensaver`, and XML in
`share/xscreensaver/config`. Override `REFLOW_XSCREENSAVER_HACK_DIR` and
`REFLOW_XSCREENSAVER_CONFIG_DIR` at CMake configure time for other installations.
`DESTDIR` staging is supported. No daemon or screen lock is started by installation.

Registration appends one entry, preserving existing effects and preferences.
It is idempotent, checks for concurrent file changes, and creates
`~/.xscreensaver.before-matrix-reflow` on its first change. For a custom install,
pass `--command /absolute/path/to/matrix-reflow`; `--config FILE` supports staging.
Close settings while using the helper so its later writes do not undo registration.
Remove the entry with:

```sh
python3 Linux/xscreensaver/register.py --remove
```

The host launches `--root` with `XSCREENSAVER_WINDOW`, or supplies `--window-id ID`.
An explicit ID takes precedence; XScreenSaver settings may supply both flags.
The existing visual is used, with single/double buffering matched to that visual.
Borrowed windows retain their owner's title, geometry and input handling.
An invalid host fails clearly, without opening another window. `--root` never
falls back to painting the actual desktop root. `--windowed` ignores the host
environment and explicitly creates a standalone window.

## Testing and diagnostics

Use `-DREFLOW_GL_TESTS=ON` to include GLX checks on the current DISPLAY. These
create only their own unmapped windows, including a host that resizes and
vanishes while borrowed. Without this option, CTest needs no display. The CPU
suite checks deterministic animation across frame rates, pause recovery, camera
recycling, settings changes, bounds and injected allocation failures.

`--hidden --frames 2 --warmup 8 --capture /tmp/rain.ppm` captures developed rain
without mapping a window. `--fps-limit 60` caps rendering; `--duration 5 --stats`
provides a bounded measurement. Reported frame work includes presentation waits,
so it is not a GPU-only timer. Captures use the final frame with `--frames`,
otherwise the first frame. SIGTERM exits normally.

## Current limits

SDR output has no bloom or CRT yet; Windows visual parity is not claimed. The
scene uses premultiplied blending and an opaque final composite. Font mipmaps
stop at level 3 to retain integral cell boundaries. Native Wayland and screen
locking/authentication are outside this port. See [iteration 2 validation](validation.md#i2-m4)
for hardware coverage, preview checks and the deliberately shortened stability run.
