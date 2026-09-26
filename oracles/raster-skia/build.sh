#!/bin/sh
# Build the Skia m144 oracle against Ladybird's libskia.
cd "$(dirname "$0")"
P=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/packages/skia_arm64-osx-dynamic
L=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic/lib
clang++ -std=c++20 -O1 -ffp-contract=off -Wall -Wno-unused -I$P/include/skia *.cpp -L$P/lib -lskia -lz -Wl,-rpath,$P/lib -Wl,-rpath,$L -o oracle
