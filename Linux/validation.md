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
