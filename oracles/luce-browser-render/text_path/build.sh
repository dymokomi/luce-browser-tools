#!/bin/sh
# Build the text-path oracle against the donor's LibGfx (flags of Build/release's compile_commands).
cd "$(dirname "$0")"
D=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
B=$D/Build/release
V=$B/vcpkg_installed/arm64-osx-dynamic
clang++ -std=c++23 -O1 -DNDEBUG -fno-exceptions -ffp-contract=off -fsigned-char -Wno-unqualified-std-cast-call -Wno-user-defined-literals -Wno-c23-extensions -I$D -I$D/Libraries -I$B -I$B/Libraries -isystem $V/include -isystem $V/include/skia -isystem $V/include/harfbuzz oracle.cpp -L$B/lib -llagom-gfx -llagom-core -llagom-ak -Wl,-rpath,$B/lib -Wl,-rpath,$V/lib -o oracle
