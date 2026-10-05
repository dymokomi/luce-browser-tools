#!/bin/sh
# Build the skcms oracles from the skcms of the Skia Ladybird pins (vcpkg buildtree):
# skcms_oracle, the portable one-lane build without contraction that luce-color's icc port
# follows to the bit, and skcms_oracle_simd, the NEON build as Skia ships it.
cd "$(dirname "$0")"
S=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/buildtrees/skia/src/41119015ca-b0525dd526.clean/modules/skcms
SRC="$S/skcms.cc $S/src/skcms_TransformBaseline.cc $S/src/skcms_TransformHsw.cc $S/src/skcms_TransformSkx.cc"
clang++ -std=c++17 -O2 -ffp-contract=off -DSKCMS_PORTABLE -I$S skcms_oracle.cpp $SRC -o skcms_oracle
clang++ -std=c++17 -O2 -I$S skcms_oracle.cpp $SRC -o skcms_oracle_simd
# skia_oracle: SkColorSpace::Make and image drawing through the libskia Ladybird links.
P=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/packages/skia_arm64-osx-dynamic
L=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic/lib
clang++ -std=c++20 -O1 -ffp-contract=off -Wall -Wno-unused -I$P/include/skia skia_oracle.cpp -L$P/lib -lskia -Wl,-rpath,$P/lib -Wl,-rpath,$L -o skia_oracle
