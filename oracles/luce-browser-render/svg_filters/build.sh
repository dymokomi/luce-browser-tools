#!/bin/sh
# Build the SVG-filter oracles against the donor's LibGfx and Skia m144: `replay` (canvas
# operations logged by the CPU player, played on SkCanvas with a named Gfx::Filter graph) and
# `probe_displacement` (sk_displacement's arithmetic read back from an F32 surface).
cd "$(dirname "$0")"
D=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
B=$D/Build/release
V=$B/vcpkg_installed/arm64-osx-dynamic
clang++ -std=c++23 -O1 -DNDEBUG -DSK_GANESH -DSK_METAL -fno-exceptions -ffp-contract=off -fsigned-char -Wno-unqualified-std-cast-call -Wno-user-defined-literals -Wno-c23-extensions \
  -I$D -I$D/Libraries -I$B -I$B/Libraries -isystem $V/include -isystem $V/include/skia replay.cpp \
  -L$B/lib -L$V/lib -llagom-gfx -llagom-core -llagom-ak -lskia -Wl,-rpath,$B/lib -Wl,-rpath,$V/lib -o replay
clang++ -std=c++20 -O1 -ffp-contract=off -isystem $V/include/skia -I$V/include/skia probe_displacement.cpp -L$V/lib -lskia -Wl,-rpath,$V/lib -o probe_displacement
