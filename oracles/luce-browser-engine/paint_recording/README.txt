r51b+r52 paint recording oracle (luce-browser-engine painting/tests_paint_recording*): the reference build's LibWeb
records the display list of small documents (dump_display_list) and renders a screenshot with DisplayListPlayerSkia
(checked identical to Ladybird --headless=screenshot --test-mode --force-cpu-painting for every case:
compare_headless.py, which writes the screenshots to ~/Downloads first, as Ladybird does, and moves them away).
  ./build.sh
  ./oracle cases.txt > expected.txt
  python3 gen_luce_cases.py > .../web/painting/tests_paint_recording_cases.lucb
r49+r53: cases svg-shapes, svg-strokes, svg-viewbox, markers, fieldset (dl and px each) appended to cases.txt.
