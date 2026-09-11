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

At this milestone, animated rendering and XScreenSaver integration were still pending.

## Iteration 2

* Animated rain uses the existing GLX glyph renderer, a fixed 60 Hz simulation,
  bounded catch-up, wall clock, depth, camera path and synchronized recycling.
* Parameter validation and transactional core allocation prevent uncontrolled
  growth and preserve existing state on allocation failure.
* Density changes refresh column spacing as well as column count.
* Borrowed X11 host windows use their existing visual; their ownership, geometry,
  title and input handling remain with the host. Vanishing windows exit cleanly.
* XScreenSaver XML, configurable install paths and an idempotent registration helper
  add the effect to the existing list without replacing other preferences.
* Actual embedded and expanded XScreenSaver previews show animated font rain.
* A default 60 FPS cap, bounded diagnostic runs and frame statistics make CPU use
  and pacing measurable; long pauses do not trigger unbounded catch-up.
* Drawable-loss handling now covers driver requests during drawing and GL teardown,
  including XScreenSaver removing a full-screen host during deactivation.
* CLI bounds, stale host environments and SIGTERM are covered by executable tests.

The planned 30-minute run was stopped at the user's request on battery. Short
checks passed; long-term memory stability and physical multi-monitor behavior
remain unverified. Bloom, CRT and the full settings tool are still pending.

## Iteration 3

* Four-tap soft-knee extraction and five RGBA16F bloom levels with reference
  13-tap downsampling, tiny-window handling and level diagnostics.
* Reference 9-tap additive upsampling and opaque SDR composition, with bloom
  strength, barrel/chromatic distortion, vignette, edge fade and startup easing.
* Glow defaults to .9; `--no-bloom`, `--bloom-strength`, `--distortion` and
  `--no-post` support tuning and comparisons. Unused bloom targets are released.
* XScreenSaver controls for bloom, glow strength, distortion and frame limit.
* Deterministic snapshot mode, owned/borrowed pixel parity checks and a bounded
  GPU/CPU benchmark. The 1080p sample measured 3.93 ms GPU time with bloom.
* Actual embedded/expanded previews were checked with a compositor; no long
  battery-draining tests were run. CRT filtering remains iteration 4.

## Iteration 4

* Optional full-size CRT intermediate and shared final-output routing, with
  on-demand allocation, immediate release on disable and identity diagnostics.
* Reference CRT RGB mask, horizontal phosphor spread, channel convergence,
  line ripple, lifted black level, highlight rolloff and vignette. The mask
  uses top-left output-pixel coordinates, independent of scene/camera motion.
* All bloom/CRT combinations work in standalone and XScreenSaver previews;
  CRT-off captures match iteration 3 byte-for-byte. CRT remains off by default.
* Short 1080p checks measured 5.78 ms GPU time with bloom and CRT, with a lazy
  15.82 MiB CRT intermediate. No long battery-draining tests were run.
* Added comparison captures, CLI/identity checks and expanded-preview validation.

## Iteration 5

* Shared typed settings schema, version-1 GLib INI storage and five named profile slots.
* Private atomic saves, strict validation and display-independent effective-settings output.
* Defaults → saved profile → explicit arguments; CLI never writes profiles.
* Schema-generated CLI help and complete positive/negative switches, including RGB channels.
* XScreenSaver profile selection and explicit-default mode, with checked numeric/boolean controls.
