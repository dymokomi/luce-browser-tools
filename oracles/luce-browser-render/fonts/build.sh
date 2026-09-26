# Font oracle of region r09 (luce-browser-render web_fonts): C++ programs linked against the
# reference build's LibGfx (Skia/FreeType under the FontConfig font manager, HarfBuzz).
#   sh build.sh gen_tests.cpp gen_tests && ./gen_tests > tests_oracle body   (see regen.sh)
#   sh build.sh survey.cpp survey && ./survey FONT... ; survey_luce/ is the Luce side; classify.py
#   compares survey_cpp_all.txt with survey_luce_all.txt.
D=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin; B=$D/Build/release; V=$B/vcpkg_installed/arm64-osx-dynamic
clang++ -w -std=c++23 -O1 -fno-exceptions -fsigned-char -DSK_GANESH -DSK_METAL -DSK_FONTMGR_FREETYPE_EMPTY_AVAILABLE -DSK_TYPEFACE_FACTORY_FREETYPE -I$D -I$D/Libraries -I$B -I$B/Libraries -isystem $V/include -isystem $V/include/skia -isystem $V/include/harfbuzz -isysroot $(xcrun --show-sdk-path) "$1" -o "$2" -L$B/lib -L$V/lib -llagom-gfx -llagom-core -llagom-ak -lskia -lharfbuzz -Wl,-rpath,$B/lib -Wl,-rpath,$V/lib 2>&1 | grep -A5 "error" | head -30
