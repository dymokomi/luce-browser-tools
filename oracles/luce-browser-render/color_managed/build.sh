#!/bin/sh
# Build the color-managed draw_bitmap oracle against the donors LibGfx and Skia m144:
#   ./oracle > scenes.txt (pixels for tests_gfx_paint_color.lucb).
cd "$(dirname "$0")"
D=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
B=$D/Build/release
V=$B/vcpkg_installed/arm64-osx-dynamic
clang++ -std=c++23 -O1 -DNDEBUG -DSK_GANESH -DSK_METAL -fno-exceptions -ffp-contract=off -fsigned-char -Wno-unqualified-std-cast-call -Wno-user-defined-literals -Wno-c23-extensions \
  -I$D -I$D/Libraries -I$B -I$B/Libraries -isystem $V/include -isystem $V/include/skia oracle.cpp \
  -L$B/lib -L$V/lib -llagom-gfx -llagom-core -llagom-ak -lskia -Wl,-rpath,$B/lib -Wl,-rpath,$V/lib -o oracle
