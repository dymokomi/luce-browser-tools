# Donor setup

The reference sources the ports compare against. Clone them into `.donors/` next to the
package repositories, for example `~/Dev/luce_dev/.donors/`. They are never copied into a
package repository.

| Donor | Where | Version | Used by |
| --- | --- | --- | --- |
| Ladybird | https://github.com/LadybirdBrowser/ladybird | `47c82b38d0e968d1f8b9e4f6cbe10d87c5401bac` (2026-05-03, the last commit before Rust entered LibWeb), checked out as `.donors/ladybird-pin` | every `luce-browser-*` package |
| Skia m144 | built by Ladybird's vcpkg: `.donors/ladybird-pin/Build/vcpkg/buildtrees/skia/src/41119015ca-b0525dd526.clean/` (source), `Build/vcpkg/packages/skia_arm64-osx-dynamic/` (library + headers) | the version Ladybird 47c82b38d0 pins | render's raster, display_list, fonts oracles |
| tiny-skia | https://github.com/linebender/tiny-skia | `5d4754777746eef0828be166896eaf482c49f8f2` | render's raster (the port started from it) |
| test262 | fetched by luce-js's `tests/test262.py` itself (commit 5c8206929d, plus QuickJS's patch) | | luce-js |

## Building Ladybird (release, with the test binaries)

```sh
cd .donors
git clone https://github.com/LadybirdBrowser/ladybird.git ladybird-pin
git -C ladybird-pin checkout 47c82b38d0e968d1f8b9e4f6cbe10d87c5401bac
cd ladybird-pin && ./Meta/ladybird.py build        # release preset → Build/release
```

This gives `Build/release/bin/` (test-web, TestString, css-tokenizer, …),
`Build/release/compile_commands.json` (for `skeleton/extract.py`), the lagom libraries the
oracles link, and vcpkg's Skia. The reference build passes 974/974 of Ladybird's Layout tests
(`Build/release/bin/test-web`).

QuickJS, the donor of luce-js, is described in luce-js's own `docs/HANDOFF.md`.
