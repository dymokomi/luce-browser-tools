r40 ComputedProperties oracle. ./build.sh builds ./oracle (links the reference build's LibWeb objects);
./oracle > expected.txt runs cases.txt (each case's line goes to stderr first, so a VERIFY names its case);
python3 gen_luce_cases.py OUT.lucb writes luce-browser-engine's css/tests_computed_properties_cases.lucb.
