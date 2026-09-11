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
