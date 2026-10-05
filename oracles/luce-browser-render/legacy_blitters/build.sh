#!/bin/sh
# Build the legacy-blitter oracle against the libskia (m144) Ladybird 47c82b38d0 links:
#   ./oracle > scenes.txt && python3 gen_data.py scenes.txt <render>/src/raster/tests_legacy_blitter_data.lucb
cd "$(dirname "$0")"
V=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic
/usr/bin/clang++ -std=c++20 -O1 -arch arm64 -ffp-contract=off -isystem $V/include/skia -I$V/include/skia oracle.cpp -L$V/lib -lskia -Wl,-rpath,$V/lib -o oracle
