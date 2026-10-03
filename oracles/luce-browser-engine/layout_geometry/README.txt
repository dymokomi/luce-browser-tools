r44 layout geometry oracle (luce-browser-engine layout/tests_layout_geometry*).
  ./build.sh                          # links oracle.cpp with the reference build's LibWeb objects
  ./oracle cases.txt > expected.txt   # layout dumps of every case
  python3 compare_ladybird.py         # the cases taken from Ladybird's tests match their expected .txt
  python3 gen_luce_cases.py > .../web/layout/tests_layout_geometry_cases.lucb
  python3 from_ladybird.py block-and-inline/NAME ...   # turn Ladybird test inputs into case lines
