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
