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
| `skeleton/` | The libclang skeleton generator that produced the typed stubs of every `luce-browser-*` package from Ladybird 47c82b38d0 (`extract.py`, then `generate.py`). **Do not re-run `generate.py` over ported repositories**: it rewrites generated fragments. Its model cache (1.4 GB) is not kept; `extract.py -j 13` rebuilds it in about 3.5 minutes from the donor's `compile_commands.json`. |
| `oracles/luce-browser-foundation/` | e.g. `string_hash_oracle.cpp`: AK hash values pinned in foundation tests. |
| `oracles/luce-browser-html/tokenizer/` | HTMLTokenizer oracle and the expected outputs pinned in html_syntax's tests. |
| `oracles/luce-browser-render/` | `fonts/` (Skia/FreeType/HarfBuzz metrics, shaping and glyph paths via Ladybird's LibGfx), `player/` (DisplayListPlayerSkia scenes), `filters/` (Skia image filters), `text_path/`. |
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
