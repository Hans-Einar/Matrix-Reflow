# Matrix Reflow on Linux

The Linux port now renders animated Matrix rain in its own window and inside
XScreenSaver's actual preview and saver windows. Both use the same OpenGL 3.3/GLX
renderer, embedded FreeType font atlas and shared C simulation. Multiscale bloom
and SDR composition are implemented, with an optional CRT filter. Configuration and preview use XScreenSaver’s existing Settings dialog.
There is no separate GTK settings application.
See the [implementation plan](implementation-plan.md) and [study](study.md).

## Build and run

Verified on AlmaLinux 10.2 with `gcc gcc-c++ cmake make pkgconf-pkg-config
libX11-devel libepoxy-devel freetype-devel glib2-devel` and Python 3 for registration tests.
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

## Bloom and output controls

Bloom defaults to on at strength .9. XScreenSaver's effect settings now expose
**Bloom**, **Glow strength**, **Glass distortion**, **CRT emulation** and **Frame limit**. The CLI
also supports:

```sh
matrix-reflow --bloom-strength .5 --distortion .3 --fps-limit 30
matrix-reflow --no-bloom
matrix-reflow --no-post
```

`--no-bloom` disables glow while retaining the reference vignette/edge treatment.
`--no-post` gives raw iteration-2 output. Distortion defaults to zero; bloom and
distortion ease in over 1.8 seconds. The diagnostic control/glyph scenes stay raw
unless you explicitly request a bloom, distortion or CRT option. `--bloom-level 1..5`
shows extraction before upsampling, at half resolution down to 1/32 resolution.

For reproducible captures, use `--snapshot-time 8 --seed 12345 --frames 1` with
`--capture FILE.ppm`. This freezes simulation/shimmer at the selected 60 Hz step
and fixes the clock to noon. It is a diagnostic mode, not normal animation.
[Same frame without bloom](docs/rain-no-bloom.png) and
[with bloom](docs/rain-bloom.png) show the reference .9 intensity at 1080p.

`bloom-benchmark OUTPUT-DIRECTORY` is an optional build-tree tool, never run by
CTest. It measures 12 frames per mode at 1080p and the current display size
(deduplicated if equal), using GPU elapsed-time queries and process CPU time.
The output is rendered into an RGBA8 FBO so obscured/unmapped windows cannot
skip composition. The short sample is not a power, thermal or sustained-FPS test.
See [iteration 3 validation](validation.md#i3-m3) for measurements and scope.

## CRT emulation

CRT is **off by default**. Enable **CRT emulation** in the XScreenSaver effect
settings, or try:

```sh
matrix-reflow --crt --fps-limit 30
matrix-reflow --crt --no-bloom
matrix-reflow --no-crt
```

The filter adds the reference RGB mask, horizontal phosphor spread, convergence
fringes, line ripple, lifted black level and highlight rolloff after composition.
It is independent of **Glass distortion** (`--distortion`), which models curved
glass. CRT uses output-pixel coordinates, so the fine mask should be inspected
at 100% image scale. It does not add history-based afterglow.

`--no-post` bypasses both composition effects and CRT. `--crt-identity` exercises
the intermediate through a neutral pass for diagnostics. A full-size FP16
intermediate exists only while CRT is active (about 15.82 MiB at 1080p).
`bloom-benchmark` now also measures CRT alone and CRT with bloom.

Compare [CRT alone](docs/rain-crt.png), [CRT + bloom](docs/rain-crt-bloom.png),
[bloom alone](docs/rain-bloom.png) and [neither](docs/rain-no-bloom.png), at the
same seed/time. See [iteration 4 validation](validation.md#i4-m3) for reference
quirks, GPU measurements and actual preview checks.

## Current limits

Windows visual parity is not claimed. The
scene uses premultiplied blending and an opaque final composite. Font mipmaps
stop at level 3 to retain integral cell boundaries. Native Wayland and screen
locking/authentication are outside this port. See [iteration 2 validation](validation.md#i2-m4)
for hardware coverage, preview checks and the deliberately shortened stability run.

## Profiles and command-line settings

`matrix-reflow` loads the active saved profile, then applies explicit arguments.
`--profile 1` through `--profile 5` choose stable slots regardless of their names.
`--profile 0` or `--no-config` uses built-in defaults without reading the file.
`--no-config` conflicts with a nonzero profile. Argument position does not change
precedence; repeated individual switches use the last value. CLI never saves.

Profiles use version-1 GLib INI at `$XDG_CONFIG_HOME/matrix-reflow/settings.ini`
(or `~/.config/matrix-reflow/settings.ini`). Missing files use defaults; malformed
values and unsupported versions produce an error instead of silently replacing
settings. Saves atomically replace the file with private permissions. Keep a
backup before manually editing; simultaneous editors currently use last save wins.

```sh
matrix-reflow --profile 2 --crt --fps-limit 30
matrix-reflow --no-config --main-red 0.05 --main-green 0.85 --main-blue 0.25
matrix-reflow --profile 2 --print-effective-settings
matrix-reflow --help
```

Help is generated from the shared schema and includes every supported setting,
range and default. Each boolean has both `--NAME` and `--no-NAME`. Colors use
separate `--main-red/green/blue` and `--glitch-red/green/blue` channels, each 0..1.
Effective-settings output lists the base profile, explicit overrides and resolved
values without opening a display. Raw-rendering diagnostics (`--no-post`, control,
glyph lab, snapshot time and identity CRT) remain per-run CLI options, not profiles.

XScreenSaver’s existing **Settings** dialog is the configuration UI. It exposes
all supported fields, including six RGB channel controls (0..1), and uses its
existing preview. It always emits `--profile 0` so displayed defaults are accurate
and optional CLI profiles cannot silently change the effect. No separate GTK
application is built or installed. Restart the settings dialog after an XML upgrade.

Versioned profile storage remains available to CLI users who maintain an INI file;
there is no profile editor in this release. For example:

```ini
[Settings]
version=1
selected=1
[Profile 1]
name=Green
speed=0.35
[Profile 2]
name=Two
[Profile 3]
name=Three
[Profile 4]
name=Four
[Profile 5]
name=Five
```

Omitted fields use defaults; `--profile 1..5` selects a slot for standalone CLI
runs. GUI configuration is stored by XScreenSaver in its usual configuration file.

The ineffective upstream `waves` toggle is intentionally absent. Real display HDR,
Windows battery policy, and an on-screen FPS overlay are not implemented; use the
Linux FPS cap and `--stats`. Existing reference clock easter eggs, glyph flips,
wireframe, texture, fog, camera, gaps, bright heads, bloom and CRT are configurable.
