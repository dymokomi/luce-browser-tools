#!/bin/bash
# Build the box-shadow oracle: outer and inner box shadows through the blur mask filter,
# played by the donor's DisplayListPlayerSkia (Skia m144), printed as pixels (./oracle >
# scenes.txt; python3 gen_data.py scenes.txt <render>/src/display_list/tests_skia_reference_data_7.lucb).
# The LibWeb painting objects are built once into $OBJ (outside the repository).
set -e
cd "$(dirname "$0")"
B=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release
S=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
OBJ=${OBJ:-/tmp/luce-browser-oracle-obj}
FLAGS="-DENABLE_COMPILETIME_FORMAT_CHECK -DSK_GANESH -DSK_METAL -I$S -I$S/Libraries -I$S/Services -I$B -I$B/Libraries -I$B/Services -isystem $B/vcpkg_installed/arm64-osx-dynamic/include -isystem $B/vcpkg_installed/arm64-osx-dynamic/include/skia -O1 -std=c++23 -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk -mmacosx-version-min=14.0 -fno-exceptions -ffp-contract=off -fsigned-char -Wno-user-defined-literals -fconstexpr-steps=16777216 -Wno-c23-extensions -fvisibility=hidden"
mkdir -p $OBJ
OBJECTS=
for f in BorderRadiiData DisplayList AccumulatedVisualContext DisplayListPlayerSkia DisplayListCommand DisplayListRecorder ScrollState ExternalContentSource PaintStyle; do
  if [ ! -f $OBJ/$f.o ]; then /usr/bin/clang++ $FLAGS -c $S/Libraries/LibWeb/Painting/$f.cpp -o $OBJ/$f.o; fi
  OBJECTS="$OBJECTS $OBJ/$f.o"
done
/usr/bin/clang++ $FLAGS -c oracle.cpp -o $OBJ/shadows_driver.o
/usr/bin/clang++ -arch arm64 $OBJ/shadows_driver.o $OBJECTS -L$B/lib -L$B/vcpkg_installed/arm64-osx-dynamic/lib -llagom-ak -llagom-gfx -llagom-core -llagom-gc -lskia -lharfbuzz -Wl,-rpath,$B/lib -Wl,-rpath,$B/vcpkg_installed/arm64-osx-dynamic/lib -o oracle
V=$B/vcpkg_installed/arm64-osx-dynamic
