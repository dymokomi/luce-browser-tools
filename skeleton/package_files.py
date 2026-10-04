"""package.prisma, README.md, LICENSE, PIN and test.sh of each luce-browser package."""
import os

import scope as S
from clangenv import PIN

DESCRIPTIONS = {
    'foundation': 'The foundations of the luce-browser port of Ladybird: the AK subset, LibGC, the LibUnicode '
                  'pieces LibWeb uses, LibTextCodec, LibURL and the WHATWG Infra helpers, in luce-base.',
    'css': 'CSS Syntax Level 3 below the object model (tokenizer, tokens, component values) and the generated '
           'CSS data tables, from Ladybird\'s LibWeb, in luce-base.',
    'html': 'The HTML tokenizer and named character references from Ladybird\'s LibWeb, in luce-base.',
    'render': 'Geometry, color, fonts, the CPU rasterizer and the display list of the luce-browser port of '
              'Ladybird, in luce-base.',
    'engine': 'The web engine of the luce-browser port of Ladybird: DOM, HTML, CSS, SVG, layout and painting, '
              'in luce-base.',
}

LICENSE = '''BSD 2-Clause License

luce-browser is a port of Ladybird's LibWeb and the libraries it needs to luce-base. It is a
derived work of Ladybird.

Copyright (c) 2018-2025, the Ladybird developers.
Copyright (c) 2026 the luce-browser authors.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
'''


def prisma(package):
    mods = ', '.join(f'"{m}"' for m in S.MODULES if S.PACKAGE_OF[m] == package)
    # the flat layout: modules live in src/<module>/; `public` lists those other packages may import
    lines = ['#prisma 4.0', f'def package "luce-browser-{package}" {{',
             '    str owner = "dymokomi"', '    str version = "0.1.0"', '    str kind = "package"',
             '    str language = "luce-base"', f'    str description = "{DESCRIPTIONS[package]}"',
             '    str readme = "README.md"', f'    str[] public = [{mods}]', '']
    lines += ['    def dependency "luce-std" {', '        str owner = "dymokomi"', '        str version = "^0.1.0"',
              '        str path = "../luce-std"', '    }']
    for dep in S.PACKAGE_DEPS[package]:
        lines += ['', f'    def dependency "luce-browser-{dep}" {{', '        str owner = "dymokomi"',
                  '        str version = "^0.1.0"', f'        str path = "../luce-browser-{dep}"', '    }']
    lines.append('}')
    return '\n'.join(lines) + '\n'


def readme(package):
    mods = [m for m in S.MODULES if S.PACKAGE_OF[m] == package]
    deps = ', '.join(['luce-std'] + [f'luce-browser-{d}' for d in S.PACKAGE_DEPS[package]])
    design = 'docs/DESIGN.md' if package == 'engine' else '../luce-browser-engine/docs/DESIGN.md'
    return f'''# luce-browser-{package}

{DESCRIPTIONS[package]}

Part of the luce-browser family, a faithful port of Ladybird's LibWeb to luce-base; the design every
porter follows is [DESIGN.md]({design}).

| Module | Contents |
| --- | --- |
''' + ''.join(f'| `{m}` | {MODULE_ROWS.get(m, "")} |\n' for m in mods) + f'''
Depends on: {deps}.

## Status

Skeleton: every type of the phase-1 closure is declared and every function has its generated
signature and a `trap("unported: ...")` body, grouped by region (`docs/regions.tsv`). Regions
replace their stub fragments with ported code (DESIGN.md §4.5). `docs/namemap.tsv` maps every C++
name to its Luce name; `docs/gc_fields.tsv` lists the GC pointer fields each cell must visit.

The skeleton is generated from Ladybird at `{PIN}` (see `PIN`) by a local tool that is not part
of this repository.

## Testing

`./test.sh` type-checks every module.

## License

BSD-2-Clause, as Ladybird; see `LICENSE`.
'''


MODULE_ROWS = {
    'ak': 'the AK subset LibWeb uses', 'gc': 'LibGC', 'web_unicode': 'the LibUnicode pieces LibWeb uses',
    'text_codec': 'LibTextCodec', 'web_url': 'LibURL', 'web_infra': 'WHATWG Infra helpers',
    'css_syntax': 'CSS tokenizer, tokens, component values', 'css_data': 'generated CSS data',
    'html_syntax': 'HTML tokenizer and entities', 'gfx': 'LibGfx geometry, color, paths; CSS pixels',
    'web_fonts': 'fonts and text layout', 'raster': 'the CPU rasterizer', 'display_list': 'the display list',
    'web': 'DOM, HTML, CSS, SVG, layout, painting',
}


def test_sh(package):
    mods = [m for m in S.MODULES if S.PACKAGE_OF[m] == package and m not in S.FOREIGN_MODULES]
    foreign = [m for m in S.MODULES if S.PACKAGE_OF[m] == package and m in S.FOREIGN_MODULES]
    return f'''#!/bin/sh
# Type-check every module of luce-browser-{package} (and run its tests once there are any).
# Stops at the first failing step.
set -e
cd "$(dirname "$0")"

for module in {' '.join(mods)}; do
    echo "== luce-base check src/$module -W"
    luce-base check "src/$module" -W
done
''' + ''.join(f'''
# {m}: written by hand (not generated); run its tests once it exists.
if [ -f src/{m}/ORDER ]; then
    echo "== luce-base test src/{m}"
    luce-base test src/{m}
fi
''' for m in foreign)


def write(out, package, force=False):
    files = {
        'package.prisma': prisma(package),
        'README.md': readme(package),
        'LICENSE': LICENSE,
        'PIN': f'Ladybird {PIN} (2026-05-03, "LibJS: Range-check enum-typed bytecode fields in the validator")\n',
        'test.sh': test_sh(package),
        '.gitignore': '.luce-*/\nbuild/\n',
    }
    for name, text in files.items():
        path = os.path.join(out, name)
        if os.path.exists(path) and not force:
            continue
        with open(path, 'w') as f:
            f.write(text)
        if name == 'test.sh':
            os.chmod(path, 0o755)
