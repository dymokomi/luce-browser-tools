r44 layout geometry oracle (luce-browser-engine layout/tests_layout_geometry*), extended by r45 (inline layout,
layout/tests_layout_inline*). The oracle loads only the engine's tests/libweb/fonts with the FontPlugin in layout test
mode and names its font provider "FontConfig", so TypefaceSkia uses FreeType as Ladybird's test-web does.
  ./build.sh                          # links oracle.cpp with the reference build's LibWeb objects
  ./oracle cases.txt > expected.txt   # layout dumps of every case (r44: block layout, no text)
  python3 compare_ladybird.py         # the cases taken from Ladybird's tests match their expected .txt
  python3 gen_luce_cases.py > .../web/layout/tests_layout_geometry_cases.lucb
  python3 from_ladybird.py block-and-inline/NAME ...   # turn Ladybird test inputs into case lines
r45:
  ./oracle cases_inline.txt > expected_inline.txt      # modes: layout (Dump) and lines (line boxes before commit)
  python3 compare_ladybird.py cases_inline.txt expected_inline.txt
  python3 gen_luce_cases.py inline > .../web/layout/tests_layout_inline_cases.lucb
  python3 own_cases.py                # r45's own cases (appended to cases_inline.txt)
  python3 mark_later.py NAME ...      # mark cases that wait for other regions with "!"
