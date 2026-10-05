#!/bin/sh
# Build the APNG oracle against the libpng (with the APNG patch) Ladybird's vcpkg tree installs.
cd "$(dirname "$0")"
V=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic
clang -O1 -ffp-contract=off -I$V/include apng_oracle.c -L$V/lib -lpng16 -lz -Wl,-rpath,$V/lib -o apng_oracle
