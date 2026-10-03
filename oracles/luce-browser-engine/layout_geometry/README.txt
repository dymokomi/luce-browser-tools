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
r50 (paintables): the "layout" mode now prints the three dumps of a Layout test (layout tree, paint tree, stacking
context tree) after the rest of Document::update_layout (viewport rect broadcast, scroll frames, paint and hit testing
properties); every case taken from Ladybird matches its whole expected file.
  ./oracle cases.txt > expected.txt; ./oracle cases_inline.txt > expected_inline.txt
  python3 compare_ladybird.py --full [CASES EXPECTED]   # whole expected files
  python3 mark_paintables.py CASES EXPECTED             # "!" on cases whose paint tree needs region r53's paintables
  python3 count_ladybird.py cases.txt cases_inline.txt  # Ladybird tests the port runs (and so matches) / waiting
  python3 own_paint_cases.py; ./oracle cases_paint.txt > expected_paint.txt   # modes paint, paintables, selection
  python3 gen_luce_cases.py paint > .../web/painting/tests_paintables_cases.lucb
r48 (table layout): cases_table.txt holds the 113 Ladybird Layout tests whose layout first stopped at the TableWrapper
(from_ladybird.py; the 5 that also need grid layout are marked "!") and own_table_cases.py's own cases. Modes "table"
(TableGrid::calculate_row_column_grid of every table box: rows, cells, occupied slots in HashMap order) and
"border-specificity" (TableFormattingContext::border_is_less_specific over pairs of borders).
  python3 own_table_cases.py; ./oracle cases_table.txt > expected_table.txt
  python3 compare_ladybird.py --full cases_table.txt expected_table.txt   # all 113 reproduce Ladybird's expected files
  python3 gen_luce_cases.py table > .../web/layout/tests_layout_table_cases.lucb
