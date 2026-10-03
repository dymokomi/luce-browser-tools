r41 CSS fonts oracle. ./build.sh builds ./oracle (links the reference build's LibWeb objects);
./oracle > expected.txt runs cases.txt (each case's line goes to stderr first, so a VERIFY names its case);
python3 gen_luce_cases.py OUT.lucb writes luce-browser-engine's css/tests_fonts_cases.lucb.
The fonts are those of luce-browser-engine's tests/libweb/fonts (g_fonts_directory in oracle.cpp).
