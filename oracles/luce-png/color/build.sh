#!/bin/sh
# Build the libpng colour-chunk oracle against the libpng Ladybird's vcpkg tree installs.
cd "$(dirname "$0")"
V=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic
clang -O1 -I$V/include libpng_oracle.c -L$V/lib -lpng16 -lz -Wl,-rpath,$V/lib -o libpng_oracle
