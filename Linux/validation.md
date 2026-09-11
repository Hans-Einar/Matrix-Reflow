# Linux validation

Results are milestone-specific; future milestones are not implied to pass.

## Environment

AlmaLinux 10.2; GCC/G++ 14.3.1; CMake 3.31.8; X11 1.8.10;
libepoxy 1.5.10; FreeType package 2.13.2. GPU: Intel HD 620, Mesa 25.2.7.

## I1-M1

2026-09-11: clean Debug and Release builds passed with GCC, without warnings.
CTest core-smoke passed in both (0.06/0.02 s). The executable links the C core
and reports its default density. A fresh configure with Freetype disabled failed
with an explicit REQUIRED-package diagnostic, as expected. Tests remain active
in Release (no reliance on disabled assert()).

## I1-M2

2026-09-11: Debug and Release atlas tests passed. All 57 cells contain glyph
coverage, unused cells and all cell borders are empty. Fixed indices for 0/1/colon
are checked. Synthetic bitmaps verify positive/negative padded pitch and reject
bad bounds. Missing/corrupt font data produces controlled errors.
The 1152×288 PGM atlas was inspected visually via a lossless PNG conversion:
no clipped glyphs or cell overlap; glyph shapes follow the supplied font.
Font bytes are embedded from the existing TTF at build time, without installing
the font or modifying the Windows sources. No system fallback is used.

Reference: [FreeType bitmap pitch contract](https://freetype.org/freetype2/docs/reference/ft2-basic_types.html#ft_bitmap).

## I1-M3

2026-09-11: Debug and Release builds and CPU tests passed. graphics-test passed
on the actual Intel HD 620 (GL 4.6) and Debug also on a private Xvfb/llvmpipe
display (GL 4.5). It checks 30 actual window/FBO resizes, RGBA readback orientation,
color transfer and opaque alpha; zero-size handling, WM_DELETE_WINDOW and a
deliberately invalid shader with source/stage diagnostics.
A windowed control-scene capture was produced in the private Xvfb session.
The real-session tests used only their own unmapped windows. No mouse input,
Window Maker restart or screen lock was used.

A nonexistent DISPLAY failed cleanly. MESA_GL_VERSION_OVERRIDE=3.0 caused a
controlled context-creation failure, with no Xlib fatal termination. X11 resize
requests are asynchronous under Window Maker; the test waits for the resulting
ConfigureNotify rather than assuming XSync means the WM has completed the resize.

Reference: [GLX_ARB_create_context](https://registry.khronos.org/OpenGL/extensions/ARB/GLX_ARB_create_context.txt).

## I1-M4

2026-09-11: all four CTest tests passed in Debug and Release on Intel HD 620.
The GLX and glyph tests also passed with a private Xvfb/llvmpipe context forced
to OpenGL 3.3 / GLSL 330. GPU tests compare all 57 characters with the actual
FreeType atlas at native cell resolution in normal/X/Y/XY modes; maximum
8-bit channel error was 1 (tolerance 4). Colors, final alpha, empty scenes and
missing input are checked. Size/offset static assertions enforce the core's
40-byte instance layout (attributes at 0/16/28).

The mapped glyph lab ran on the actual Intel display and the result was visually
inspected at original resolution: all glyphs, four flip/color groups and brightness
levels are present. [Captured frame](docs/glyph-lab.png) is a lossless conversion
from the program's PPM capture. Captures use the same composite shader in a
temporary RGBA8 framebuffer, so readback is independent of window occlusion.
Capture is once per invocation (last requested frame, or first for an unbounded run).

A staged Release installation under a temporary prefix ran from `/tmp`, outside
the source/build tree. Its atlas dump worked without DISPLAY. Invalid numeric
arguments, missing values and unsupported `--root` failed clearly. No global
installation, screensaver registration or startup changes were performed.

The permanent renderer accepts MMGlyphInstance arrays and an explicit camera/view;
the glyph lab only supplies static data. Animated simulation, borrowed windows
and XScreenSaver preview are iteration 2. Bloom/CRT are later iterations.
This is not a Windows image-parity claim, an ASan run or a multi-monitor test.

## I2-M1

2026-09-11: Release tests passed. Simulation tests compare identical seeds at
30 and 120 render FPS, including two camera recycling boundaries, depth 0/1.5,
16:9/32:9/portrait, finite projection, sorted glyphs, resize and parameter updates.
A synthetic 600-second pause advances only six physics steps. Allocation fault
injection covers all nine creation allocations and all eight growth allocations;
failed growth retains byte-identical old instances. Linux validates settings and
checks the shared 1,048,576-instance ceiling before allocation.
The density-only world-spacing refresh was corrected in the shared core.
The existing void update API remains available; Linux uses the new checked API.
An 8-second simulated rain frame was captured and inspected on Intel HD 620.
No bloom or CRT was used.

## I2-M2

2026-09-11: borrowed-window-test passed on Intel HD 620: twelve asynchronous
owner-driven resizes, compatible-visual rendering, preserved host title/window,
rejected borrower resize, host destruction while GLX is alive, and invalid/root/
InputOnly windows. GLX presentation and destruction trap raced-away drawables
instead of allowing Xlib's default fatal error handler to terminate the process.
The actual host visual determines FBConfig; single-buffered visuals use front
buffer rendering and flush, double-buffered visuals use swap.
XScreenSaver 6.16 appends --window-id to the configured --root command, so those
flags are intentionally compatible (explicit ID wins). --windowed remains exclusive.

## I2-M3

2026-09-11: installed into `/usr/local` alongside XScreenSaver 6.16. The XML
was loaded by the actual settings application, which launched the installed
binary with `--root --window-id`; the expanded preview used `--root` and the
host environment variable. Both showed changing frames through the same renderer.
GUI verification used AT-SPI against only the private settings process on Xvfb
(GLX llvmpipe, Mesa 25.2.7), without operating the real desktop or its lock.
[Embedded preview readback](docs/xscreensaver-preview.png) is from the real
681x382 settings preview window; the expanded host was 1280x800. Two snapshots
300 ms apart differed in each mode. On this local XScreenSaver build the demo
inherits locking preferences, so the final private preview check temporarily
disabled lock, then restored the original lock/selection/mode/geometry fields.

Twenty alternating selections of Matrix Reflow/XMatrix each reached one live
renderer and replaced the previous PID. The expanded preview deactivated and
closing settings left no renderer behind. This exposed a drawable-loss race
outside `present()` on software GL; I2-M4 covers that additional draw-phase case.
Registration tests cover repeated add/remove, multiline entries, preferences,
custom paths with spaces and selection-index correction when removing an entry.
The real configuration retains XMatrix as selected, with locking unchanged.
Removal and install-path overrides are documented in the README.

## I2-M4

2026-09-11: Release's eight existing CTest tests passed (13.51 seconds total).
After the drawable-loss correction, all three GL tests passed again on Intel
HD 620. Debug built successfully and seven tests passed; the longer simulation
suite was not repeated in Debug to save battery. The new executable lifecycle
test passed separately in Release and Debug: invalid/NaN/out-of-range arguments,
missing values, stale host environment, explicit window mode, 10 FPS cap,
bounded duration and normal SIGTERM shutdown. It is registered with the optional
GL tests, bringing the full suite to nine tests.

The final installed binary was tested again through actual XScreenSaver settings
and its expanded preview on private Xvfb. Deactivation, closing settings and
exiting the private daemon left no renderer processes. The previous software-GL
`BadDrawable` during `X_GetGeometry` is fixed: a nested lifetime error guard now
covers implicit driver requests during drawing and GL object destruction, not
only swap. The regression test deliberately draws after the owner destroys the
host. No such X error appeared in the final expanded-preview run. The real
session was not restarted and all original lock/selection preferences remain.

### Short performance baseline

The user cancelled the planned 30-minute run because the machine was on battery;
it was stopped with SIGTERM and **not restarted**. The already collected sample
on Intel HD 620 / Mesa 25.2.7 was 63.9424 seconds, 3799 frames, using an unmapped
1152x640 owned window and 60 FPS cap, without bloom/CRT. This is a preliminary
baseline from the pacing build before the final drawable-loss guard:

| Measurement | Observed |
| --- | --- |
| Interval FPS | 59.37–59.56 |
| Mean frame work, including presentation | 2.33–3.97 ms |
| Highest observed frame work | 18.90 ms |
| RSS at every sample from 10–60 seconds | 55,336 KiB |
| CPU, 10–60 seconds | 9.07 CPU seconds / 50.01 wall seconds, about 18.1% of one core |
| Dropped simulation time | 0 seconds |

These short checks do **not** establish long-term memory stability or sustained
thermal/power behavior. The original 30-minute acceptance check was explicitly
waived. Physical multi-monitor operation, real suspend/resume, Windows rendering
parity, bloom and CRT remain untested or deferred. Synthetic aspect ratios and
a 600-second simulation pause are covered, but are not substitutes for hardware
multi-monitor or suspend testing.

## I3-M1

2026-09-11: Release bloom/graphics/glyph tests passed on Intel HD 620 (0.87 s).
The reference four-tap extraction uses threshold .72*.8, soft knee .3 and gain
1.5. The following 13-tap kernels retain normalized weights. Float readback
checks constant dark/mid/bright inputs, HDR retention (1.5), all five dimensions,
a bright patch spreading with decreasing peak, and 1x1/odd/narrow resizes.
Each level owns a distinct RGBA16F texture and complete framebuffer; replacement
allocation is transactional. A pass rejects source/destination aliasing.
`--bloom-level 1..5` displays an extracted level through the normal composite;
a one-frame level-5 diagnostic ran successfully. No-bloom glyph/control tests
retain their original pixel values. Kernels follow `windows/shaders.hlsl` in
this repository; no external Windows screenshot parity is claimed.

## I3-M2

2026-09-11: Release bloom/composite, original graphics/glyph and CLI tests passed
on Intel HD 620 in 1.85 seconds. The 9-tap tent upsample uses ONE/ONE blending
into each existing larger level without sampling that destination. Constant
bright input sums to 7.5 (five contributions of 1.5), proving that upsampling
retains the previously extracted levels. Other checks cover monotonic intensity,
zero-intensity/off equivalence, opaque alpha, repeated capture without repeated
accumulation, startup easing, invalid settings, tiny resize and runtime toggling.

The composite ports reference barrel warp, chromatic offsets, vignette and edge
fade. At full distortion the corners are black rather than stretched edge texels.
The 1.8-second smoothstep ramps bloom and distortion, as in Windows DrawPost;
it does not introduce an unrelated whole-scene fade. The SDR reference swapchain
is B8G8R8A8_UNORM, not an sRGB view. The port retains FP16 arithmetic then UNORM
clamping without an additional gamma/tone-map pass; a center-color check detects
unexpected gamma changes. GLSL keeps output alpha 1 by design. No changes were
made to the existing inactive whiteFlash term or glyph shader.

The application defaults to bloom at .9, with distortion 0. `--no-bloom` disables
only glow; `--no-post` retains iteration-2 raw output for comparisons. Diagnostic
control/glyph scenes stay raw unless bloom/distortion is explicitly requested.
Bloom textures are released when unused; level inspection shows extraction
before accumulation. Final visual/preview and cost checks follow in I3-M3.

## I3-M3

2026-09-11, AlmaLinux 10.2, Intel HD 620, Mesa 25.2.7. Actual display resolution
is **1920x1080**, so the actual-size and 1080p measurements are the same case.
No 4K screen was available and no 4K workload was added on battery. All builds
used at most two jobs. No long-running stability test was performed.

### Quality and integration

[Without bloom](docs/rain-no-bloom.png) and [with bloom](docs/rain-bloom.png)
are lossless readbacks at seed 12345, exactly 8 simulated seconds, fixed noon
clock, 9,194 instances and default settings. Both were visually inspected:
bright strokes gain a soft halo, dark columns remain visible and the corners
fade without clamp streaks. The bloom test also checks the distortion extremes.
These are Linux comparisons, not claimed Windows reference parity.

The owned/borrowed host test gives byte-identical results for bloom on/off,
strength zero and full distortion at 1x1, 3x5, 320x180 and 681x382. Owner-driven
resize and destruction tests now run with bloom/warp enabled. Release's ten
short tests passed in 4.44 seconds; the unchanged simulation test was covered
in iteration 2 and not repeated on battery. Debug built successfully and the
four focused bloom/host/lifecycle tests passed in 3.89 seconds. CLI snapshots
match across different frame counts; invalid effect values are rejected.

Actual XScreenSaver 6.16 settings were exercised on private Xvfb/llvmpipe with
`xcompmgr -n`, including more than twenty effect selections across bounded runs,
the Bloom checkbox off/on, and expanded preview. The final XML-controlled run
used a 20 FPS cap. [Actual embedded preview](docs/bloom-xscreensaver-preview.png)
is read from the 681x382 host. The checkbox launched `--no-bloom` when unchecked
and removed it when checked; the expanded host rendered the same installed
executable. Closing/deactivating left no renderer, private daemon or test server.
The live desktop was not restarted or locked, and its original lock/selection
preferences were restored after the isolated checks.

One initial automation run hit its four-second switch deadline while software
GL and a build were active; the harness was given more startup time and the
remaining checks ran after the build. No Matrix Reflow crash was observed.
XScreenSaver drops options absent from XML, so a Frame limit control was added;
fractional XML ranges/defaults also prevent the 0..1 distortion slider from
being interpreted as integral. No XML warnings appeared in the final run.

### Bounded rendering cost

`build/release/bloom-benchmark build/bloom` renders a frozen scene, warms each
mode for three frames and measures twelve. GPU time uses GL_TIME_ELAPSED; CPU
time uses CLOCK_PROCESS_CPUTIME_ID (including driver threads). Submission wall
time covers the draw calls. The persistent RGBA8 output forces full-frame
composition without X11 pixel-ownership clipping. Queries are read synchronously
between frames; this is render-only cost, excluding simulation, swap, compositor
and FPS pacing, not achievable application FPS. The raw mode is the iteration-2
rendering behavior in the new binary, not a separately rebuilt old executable.

| 1920x1080 mode | GPU mean | GPU max | CPU per frame | Draw submission wall |
| --- | ---: | ---: | ---: | ---: |
| Raw (`--no-post`) | 1.710 ms | 1.825 ms | 0.302 ms | 0.131 ms |
| Composition, no bloom | 2.032 ms | 2.092 ms | 0.256 ms | 0.125 ms |
| Composition + bloom .9 | 3.931 ms | 4.195 ms | 0.346 ms | 0.172 ms |

Bloom adds about 1.90 ms GPU time over composition alone in this small sample.
This does not justify an extra quality mode yet, so all five reference levels
are retained. Their FP16 pixel storage is about 5.27 MiB at 1080p (allocation
arithmetic, not a measured process/GPU residency value), and is released when
bloom is unused. Default frame pacing remains 60 FPS; users can choose a lower
Frame limit on battery. Sustained power/thermal behavior, other GPUs, physical
multi-monitor, 4K, HDR display output and Windows visual parity remain unverified.
