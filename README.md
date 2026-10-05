# luce-browser-tools

Local tooling behind the Luce port of Ladybird (the `luce-browser-*` packages; the JavaScript
engine is [luce-js](https://github.com/dymokomi/luce-js)). These are not part of the ported packages: the
package repositories hold only Luce code and the tests their CI runs. The C/C++ reference code
("donors") is only a guide, used for comparisons during development. This repository keeps the
comparison drivers ("oracles"), the Ladybird skeleton generator, the compiler-bug reductions and
the agent briefs, so the work can move between machines.

## Layout

| Path | What |
| --- | --- |
| `skeleton/` | The libclang skeleton generator that produced the typed stubs of every `luce-browser-*` package from Ladybird 47c82b38d0 (`extract.py`, then `generate.py`), and the phase-2 skeleton of the engine. It writes a scratch tree (`--out`, default `cache/out`), never a package repository; for a later phase it plans against the ported engine (`--baseline`, `--lower`: names and class ids, `baseline.py`) and `merge.py` brings over only what the phase adds (see `generate.py`'s docstring and DESIGN.md §4.4 "Later phases"). Its model cache (`cache/`, not kept) is rebuilt by `extract.py -j 13` in about 4 minutes from the donor's `compile_commands.json`. |
| `oracles/luce-browser-foundation/` | e.g. `string_hash_oracle.cpp`: AK hash values pinned in foundation tests. |
| `oracles/luce-browser-html/tokenizer/` | HTMLTokenizer oracle and the expected outputs pinned in html_syntax's tests. |
| `oracles/luce-browser-render/` | `fonts/` (Skia/FreeType/HarfBuzz metrics, shaping and glyph paths via Ladybird's LibGfx), `player/` (DisplayListPlayerSkia scenes), `filters/` (Skia image filters), `text_path/`. |
| `oracles/luce-color/icc/` | `skcms_oracle.cpp`: skcms (the Skia pin's) dumping profiles as luce-color's `tests/icc_tool.lucb` does, built portable without contraction (the port's bit-exact reference) and as NEON; `skia_oracle.cpp`: SkColorSpace::Make, Ladybird's load_from_icc_bytes and images drawn from color spaces through libskia; `compare.py` holds two dumps against each other with tolerances. |
| `oracles/luce-png/color/` | `libpng_oracle.c`: the color chunks (cICP, iCCP and its profile, sRGB, gAMA, cHRM) libpng 1.6.50 reads, as luce-png's `tests/color_tool.lucb` prints them. |
| `oracles/luce-jpeg/icc/` | `libjpeg_oracle.c`: the ICC profile libjpeg-turbo's jpeg_read_icc_profile joins, read as Ladybird's JPEGLoader reads it, as luce-jpeg's `tests/icc_tool.lucb` prints it. |
| `oracles/raster-skia/` | Renders the raster module's scenes with Skia m144 (the version Ladybird builds) to produce the reference PNGs. |
| `oracles/luce-regex/` | Drivers comparing luce-regex (the RegExp engine luce-js uses) with QuickJS's C libregexp/libunicode (`oracle_driver.c` builds against a QuickJS checkout). |
| `compiler-issues/` | Minimal `.lucb` reductions of luce-base bugs found by the browser ports. The tracked list is luce-js's `docs/COMPILER-REQUESTS.md`. |
| `briefs/` | The brief every region agent was given (`region-brief-template.md`) and the integration notes of the second wave (css/html/render). |

Build scripts (`build.sh`, `regen.sh`) assume the donor checkouts described in `DONORS.md`, at
`../.donors/` next to the package repositories. Their built binaries are not committed.

## Licences

The oracles link against and follow Ladybird (BSD-2-Clause), Skia (BSD-3-Clause), tiny-skia
(BSD-3-Clause), FreeType and HarfBuzz. They are test drivers for comparison and
carry no code of those projects beyond API calls. Everything here is under the BSD-2-Clause
licence of the Luce ports unless a file says otherwise.
