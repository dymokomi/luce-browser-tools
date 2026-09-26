#!/bin/sh
# build.sh OUT prog.cpp [skia/src files relative to the Skia root...]
# Compiles a program against Skia m144's *source* (internal headers and files) plus libskia.
S=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/buildtrees/skia/src/41119015ca-b0525dd526.clean
P=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/packages/skia_arm64-osx-dynamic
L=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic/lib
out=$1; shift; prog=$1; shift
srcs=""
for f in "$@"; do srcs="$srcs $S/$f"; done
clang++ -std=c++20 -O2 -ffp-contract=off -DNDEBUG -w -I$S $prog $srcs -L$P/lib -lskia -Wl,-rpath,$P/lib -Wl,-rpath,$L -o $out
