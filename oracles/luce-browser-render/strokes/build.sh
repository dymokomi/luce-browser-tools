#!/bin/bash
# Build the stroke oracle against the libskia (m144) Ladybird 47c82b38d0 links.
set -e
cd "$(dirname "$0")"
V=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic
/usr/bin/clang++ -std=c++20 -O1 -arch arm64 -ffp-contract=off -isystem $V/include/skia -I$V/include/skia oracle.cpp -L$V/lib -lskia -Wl,-rpath,$V/lib -o oracle
