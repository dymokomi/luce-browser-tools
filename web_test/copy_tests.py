#!/usr/bin/env python3
"""Copy the part of Ladybird's Tests/LibWeb that luce-browser-engine's web_test runs into the engine's
tests/libweb (DESIGN.md §6.8): the Layout, Ref, Crash and Screenshot inputs of the phases up to
--phase, with their expectations and every file the copied documents refer to.

Phase 1: an HTML document (.html/.htm) that needs neither script (no `<script`) nor resources (no
`<img`, `<iframe`, `<object`, `<embed`, `<video`, `<audio`, `<picture`, `<source`, stylesheet
`<link>`, `@import`, or `url(` other than a fragment).
Phase 2 (loading): every test document that needs no script, also those that need resources and
the XML documents (.svg, .xht, .xhtml, .xml) and the PDF the XML parser and loading bring in; the
Screenshot suite comes with phase 2.
Support files of imported WPT tests (test-web's support_file_patterns) are never tests; they are
copied when a copied document refers to them.

Usage: copy_tests.py [--phase N] [DONOR_ROOT [ENGINE_ROOT]]   (local tool, not committed)"""
import fnmatch, os, re, shutil, sys, urllib.parse

args = sys.argv[1:]
PHASE = 2
if args[:1] == ["--phase"]:
    PHASE = int(args[1])
    args = args[2:]
DONOR = args[0] if len(args) > 0 else "/Users/sedov/Dev/luce_dev/.donors/ladybird-pin"
ENGINE = args[1] if len(args) > 1 else "/Users/sedov/Dev/luce_dev/luce-browser-engine"
SRC = os.path.join(DONOR, "Tests/LibWeb")
DST = os.path.join(ENGINE, "tests/libweb")
# test-web's is_valid_test_name.
SUFFIXES = (".htm", ".html", ".svg", ".xhtml", ".xht", ".pdf")
# Documents whose text is searched for the files they refer to.
DOCUMENTS = (".html", ".htm", ".svg", ".xht", ".xhtml", ".xml", ".css")


def read(path):
    return open(path, encoding="utf-8", errors="replace").read()


def needs_script(t): return re.search(r"<script", t, re.I) is not None


def needs_resources(t):
    if re.search(r"<(img|iframe|object|embed|video|audio|picture|source)\b", t, re.I): return True
    for m in re.finditer(r"<link\b[^>]*>", t, re.I):
        if re.search(r"stylesheet", m.group(0), re.I): return True
    if re.search(r"@import", t, re.I): return True
    for m in re.finditer(r"url\(\s*['\"]?([^'\")\s]*)", t, re.I):
        if not m.group(1).startswith("#"): return True
    return False


def is_support_file(path):
    """test-web's support_file_patterns: */wpt-import/*/support/*, */wpt-import/*/resources/*,
    */wpt-import/common/*, */wpt-import/images/*."""
    return any(fnmatch.fnmatchcase(path, p) for p in ("*/wpt-import/*/support/*", "*/wpt-import/*/resources/*", "*/wpt-import/common/*", "*/wpt-import/images/*"))


def phase_of(path):
    """The phase a test document belongs to (3: it needs script), or none for a support file."""
    if is_support_file(path): return None
    if path.endswith(".pdf"): return 2
    t = read(path)
    if needs_script(t): return 3
    if path.endswith((".html", ".htm")) and not needs_resources(t): return 1
    return 2


copied = set()
added_bytes = 0
added_files = 0


def copy(path):
    """Copies `path` (a file under Tests/LibWeb) and its `.headers` file, once."""
    global added_bytes, added_files
    rel = os.path.relpath(path, SRC)
    if rel.startswith("..") or rel in copied: return False
    copied.add(rel)
    target = os.path.join(DST, rel)
    if not os.path.exists(target):
        added_bytes += os.path.getsize(path)
        added_files += 1
    os.makedirs(os.path.dirname(target), exist_ok=True)
    shutil.copyfile(path, target)
    if os.path.isfile(path + ".headers"): copy(path + ".headers")
    return True


def references(path):
    """Relative files a document refers to: match/mismatch references and other src, href,
    xlink:href, data, poster, background and srcset attributes, url(...) and @import "..."."""
    t = read(path)
    out = []
    for m in re.finditer(r"""(?<![\w-])(?:src|href|data|poster|background)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^"'\s>]+))""", t, re.I):
        out.append(m.group(1) or m.group(2) or m.group(3) or "")
    for m in re.finditer(r"""(?<![\w-])(?:srcset|imagesrcset)\s*=\s*(?:"([^"]*)"|'([^']*)')""", t, re.I):
        for candidate in (m.group(1) or m.group(2) or "").split(","):
            parts = candidate.split()
            if parts: out.append(parts[0])
    for m in re.finditer(r"""url\(\s*['"]?([^'")\s]+)""", t, re.I):
        out.append(m.group(1))
    for m in re.finditer(r"""@import\s+['"]([^'"]+)['"]""", t, re.I):
        out.append(m.group(1))
    result = []
    for ref in out:
        ref = ref.strip()
        if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", ref) or ref.startswith(("#", "/")): continue
        ref = ref.split("#")[0].split("?")[0]
        if not ref: continue
        full = os.path.normpath(os.path.join(os.path.dirname(path), urllib.parse.unquote(ref)))
        if os.path.isfile(full): result.append(full)
    return result


def copy_with_references(path):
    if not copy(path): return
    if path.endswith(DOCUMENTS):
        for ref in references(path): copy_with_references(ref)


def walk(directory):
    for d, dirs, files in os.walk(directory):
        dirs.sort()
        for f in sorted(files):
            if f.endswith(SUFFIXES): yield os.path.join(d, f)


counts = {}
for suite in ["Layout", "Ref", "Crash", "Screenshot"]:
    if suite == "Screenshot" and PHASE < 2: continue
    base = os.path.join(SRC, suite) if suite == "Crash" else os.path.join(SRC, suite, "input")
    per_phase = {1: 0, 2: 0, 3: 0}
    total = 0
    for path in walk(base):
        phase = phase_of(path)
        if phase is None: continue
        total += 1
        per_phase[phase] += 1
        if phase > PHASE: continue
        if suite in ("Layout", "Screenshot"):
            rel = os.path.relpath(path, base)
            extension = ".txt" if suite == "Layout" else ".png"
            expected = os.path.join(SRC, suite, "expected", os.path.splitext(rel)[0] + extension)
            if os.path.isfile(expected): copy(expected)
        copy_with_references(path)
    counts[suite] = (per_phase, total)
shutil.copyfile(os.path.join(SRC, "TestConfig.ini"), os.path.join(DST, "TestConfig.ini"))
# Ladybird's license beside its tests; the fonts of Assets/ carry the OFL.
shutil.copyfile(os.path.join(DONOR, "LICENSE"), os.path.join(DST, "LICENSE"))
if any(rel.startswith("Assets/") and rel.endswith((".ttf", ".otf", ".woff", ".woff2")) for rel in copied):
    copy(os.path.join(SRC, "Assets/OFL-1.1.txt"))
for suite, (per_phase, total) in counts.items():
    print(f"{suite}: {total} inputs (not counting support files): phase 1 {per_phase[1]}, phase 2 {per_phase[2]}, phase 3 {per_phase[3]}")
print(f"{len(copied)} files in the copy; {added_files} new ({added_bytes / 1e6:.1f} MB)")
