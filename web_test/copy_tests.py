#!/usr/bin/env python3
"""Copy the phase-1 part of Ladybird's Tests/LibWeb/{Layout,Ref,Crash} into luce-browser-engine's
tests/libweb (DESIGN.md §6.8), with the expectations and every file the copied tests refer to.

A test is in phase 1 when it is an HTML document (.html/.htm; XML documents need the XML parser of
phase 2) that needs neither script (no `<script`) nor resources (no `<img`, `<iframe`, `<object`,
`<embed`, `<video`, `<audio`, `<picture`, `<source`, stylesheet `<link>`, `@import`, or `url(` other
than a fragment). Usage: copy_tests.py DONOR_ROOT ENGINE_ROOT (local tool, not committed)."""
import os, re, shutil, sys

DONOR = sys.argv[1] if len(sys.argv) > 1 else "/Users/sedov/Dev/luce_dev/.donors/ladybird-pin"
ENGINE = sys.argv[2] if len(sys.argv) > 2 else "/Users/sedov/Dev/luce_dev/luce-browser-engine-r57"
SRC = os.path.join(DONOR, "Tests/LibWeb")
DST = os.path.join(ENGINE, "tests/libweb")
SUFFIXES = (".htm", ".html", ".svg", ".xhtml", ".xht", ".pdf")

def needs_script(t): return re.search(r"<script", t, re.I) is not None
def needs_resources(t):
    if re.search(r"<(img|iframe|object|embed|video|audio|picture|source)\b", t, re.I): return True
    for m in re.finditer(r"<link\b[^>]*>", t, re.I):
        if re.search(r"stylesheet", m.group(0), re.I): return True
    if re.search(r"@import", t, re.I): return True
    for m in re.finditer(r"url\(\s*['\"]?([^'\")\s]*)", t, re.I):
        if not m.group(1).startswith("#"): return True
    return False
SUPPORT = ("/wpt-import/", )
def is_support_file(path):
    """test-web's support_file_patterns: */wpt-import/*/support/*, */wpt-import/*/resources/*,
    */wpt-import/common/*, */wpt-import/images/*."""
    import fnmatch
    return any(fnmatch.fnmatchcase(path, p) for p in ("*/wpt-import/*/support/*", "*/wpt-import/*/resources/*", "*/wpt-import/common/*", "*/wpt-import/images/*"))
def is_phase1(path):
    if is_support_file(path): return False
    if not path.endswith((".html", ".htm")): return False
    t = open(path, encoding="utf-8", errors="replace").read()
    return not needs_script(t) and not needs_resources(t)

copied = set()
def copy(path):
    rel = os.path.relpath(path, SRC)
    if rel.startswith("..") or rel in copied: return False
    copied.add(rel)
    os.makedirs(os.path.dirname(os.path.join(DST, rel)), exist_ok=True)
    shutil.copyfile(path, os.path.join(DST, rel))
    return True

def references(path):
    """Relative files a document refers to (match/mismatch references, src/href, url())."""
    t = open(path, encoding="utf-8", errors="replace").read()
    out = []
    for m in re.finditer(r"""(?:src|href)\s*=\s*["']?([^"'\s>]+)""", t, re.I):
        out.append(m.group(1))
    for m in re.finditer(r"""url\(\s*['"]?([^'")\s]+)""", t, re.I):
        out.append(m.group(1))
    result = []
    for ref in out:
        if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", ref) or ref.startswith(("#", "/")): continue
        ref = ref.split("#")[0].split("?")[0]
        if not ref: continue
        full = os.path.normpath(os.path.join(os.path.dirname(path), ref))
        if os.path.isfile(full): result.append(full)
    return result

def copy_with_references(path):
    if not copy(path): return
    if path.endswith((".html", ".htm", ".svg", ".xht", ".xhtml", ".css")):
        for ref in references(path): copy_with_references(ref)

def walk(directory):
    for d, dirs, files in os.walk(directory):
        dirs.sort()
        for f in sorted(files):
            if f.endswith(SUFFIXES): yield os.path.join(d, f)

counts = {}
for suite in ["Layout", "Ref", "Crash"]:
    base = os.path.join(SRC, suite, "input") if suite != "Crash" else os.path.join(SRC, suite)
    n = total = 0
    for path in walk(base):
        total += 1
        if not is_phase1(path): continue
        n += 1
        if suite == "Layout":
            rel = os.path.relpath(path, base)
            expected = os.path.join(SRC, "Layout/expected", os.path.splitext(rel)[0] + ".txt")
            if os.path.isfile(expected): copy(expected)
            copy_with_references(path)
        else:
            copy_with_references(path)
    counts[suite] = (n, total)
shutil.copyfile(os.path.join(SRC, "TestConfig.ini"), os.path.join(DST, "TestConfig.ini"))
for suite, (n, total) in counts.items():
    print(f"{suite}: {n} of {total} inputs")
print(f"{len(copied)} files")
