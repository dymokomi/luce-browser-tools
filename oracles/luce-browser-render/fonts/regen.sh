# Regenerates the expectations of src/luce_browser_render/web_fonts/tests_oracle.lucb (keeps its
# header). The fonts are read from the r09 worktree's tests/web_fonts/fonts (gen_tests.cpp: root).
set -e
cd "$(dirname "$0")"
sh build.sh gen_tests.cpp gen_tests
./gen_tests > oracle_tests.lucb 2>/dev/null
T=${1:-/Users/sedov/Dev/luce_dev/luce-browser-render-r09/src/luce_browser_render/web_fonts/tests_oracle.lucb}
python3 - "$T" <<'PY'
import sys
t=sys.argv[1]; s=open(t).read()
i=s.index('test "face_serenity_sans"')
open(t,'w').write(s[:i]+open('oracle_tests.lucb').read())
PY
