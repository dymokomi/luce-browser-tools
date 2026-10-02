You are porting one region of Ladybird (C++) to luce-base (a C-like systems language) for luce-browser, the Luce web browser: a faithful port of Ladybird's LibWeb and the parts of AK/LibGC/LibURL/LibUnicode/LibTextCodec/LibGfx it needs, split into packages luce-browser-{foundation,css,html,render,engine}. Several agents port different regions in parallel.

YOUR REGION: REGION
PACKAGE REPO: PKG (your git worktree: WORKTREE, branch BRANCH; work only there; commit on that branch; do not push or merge; touch no other worktree or repository)

Read first, fully: /Users/sedov/Dev/luce_dev/luce-browser-engine/docs/DESIGN.md (the design — C++→Luce mappings §2, memory §3 (AK storage is on the collected heap; no destructors/free; clone() where C++ copies a mutable container), names §4.2, fragments §4.3, the region workflow §4.5, tests §6). Your package's docs/namemap.tsv, regions.tsv and the generated fragments of your module (types + your region's stub fragment `stub_*`). The language: /Users/sedov/Dev/luce_dev/luce-base/docs/language/base.md (§3–§17, §22). For the spirit of a faithful port and the stub workflow that worked before: /Users/sedov/Dev/luce_dev/luce-js/docs/PORTING.md.

Donor (read-only, local): /Users/sedov/Dev/luce_dev/.donors/ladybird-pin (Ladybird 47c82b38d0; built at Build/release, including test binaries like Build/release/bin/TestString etc. that you can run to see expected behaviour). The skeleton generator is a local tool in /Users/sedov/Dev/luce_dev/luce-browser-tools/skeleton/ — do NOT re-run generate.py over the repos (it would overwrite ported code); if a generated declaration in a types fragment is wrong for your region, fix it by hand (DESIGN §4.5 rule 5) and note it in your report.

Owner rules: repositories hold ONLY Luce code plus test data and the harness CI runs — no C++ in the repo; compare against the C++/reference build LOCALLY while developing. Commit only when the module checks (`luce-base check -W` clean) and all tests pass (the package's ./test.sh); never commit on a pipeline whose exit status isn't the test's own.

Definition of done:
- Every function of your region's stub fragment is ported faithfully into well-named fragments (header box naming the C++ files it came from, `# mark:` sections, ≤ ~600 lines, `##` docs on pub), listed in ORDER in place of the stub fragment, which is deleted. Keep C++ order and comments that explain.
- Port the donor's unit tests for your region (Tests/AK/Test*.cpp, Tests/LibURL, Tests/LibUnicode, Tests/LibTextCodec, Tests/LibGC, … whichever cover your files) into Luce `test` blocks in `tests_*.lucb` fragments (listed last in ORDER); they must pass. Where the donor has no tests, write focused tests pinned to the C++ behaviour (run the donor's code locally to get expected values). Make sure the package's test.sh runs your tests (add `luce-base test` for your module if it only checks).
- Calls into other regions' functions go to their stubs; fix a stub's SIGNATURE (not body) only if clearly wrong, and report it.
- Commit on your branch with messages ending:
SESSION_TRAILER
- Compiler: use the release named in the package's bootstrap/BASE (COMPILER is on the PATH you are given). Write the most intuitive code. When the compiler rejects or miscompiles it, reduce it to /Users/sedov/Dev/luce_dev/luce-browser-tools/compiler-issues/<name>.lucb (expected vs actual) and report it; the compiler gets fixed, the port does not work around it. Only if you are blocked, add a minimal temporary workaround tagged `# LUCE-BUG: <name>` and say so.
Final report (concise): fragments, tests ported/added and results, signature/type changes other regions must know, deviations, compiler issues, commit hash.
