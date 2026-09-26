#!/bin/bash
set -e
cd "$(dirname "$0")"
B=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release
S=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
FLAGS="-DENABLE_COMPILETIME_FORMAT_CHECK -DSK_GANESH -DSK_METAL -I$S -I$S/Libraries -I$S/Services -I$B -I$B/Libraries -I$B/Services -isystem $B/vcpkg_installed/arm64-osx-dynamic/include -isystem $B/vcpkg_installed/arm64-osx-dynamic/include/skia -O1 -std=c++23 -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk -mmacosx-version-min=14.0 -fno-exceptions -ffp-contract=off -fsigned-char -Wno-user-defined-literals -fconstexpr-steps=16777216 -Wno-c23-extensions -fvisibility=hidden"
mkdir -p obj
for f in BorderRadiiData DisplayList AccumulatedVisualContext DisplayListPlayerSkia DisplayListCommand DisplayListRecorder ScrollState ExternalContentSource PaintStyle; do
  if [ ! -f obj/$f.o ]; then /usr/bin/clang++ $FLAGS -c $S/Libraries/LibWeb/Painting/$f.cpp -o obj/$f.o; fi
done
/usr/bin/clang++ $FLAGS -c driver.cpp -o drv.o
/usr/bin/clang++ -arch arm64 drv.o obj/*.o -L$B/lib -L$B/vcpkg_installed/arm64-osx-dynamic/lib -llagom-ak -llagom-gfx -llagom-core -llagom-gc -lskia -lharfbuzz -Wl,-rpath,$B/lib -Wl,-rpath,$B/vcpkg_installed/arm64-osx-dynamic/lib -o driver
