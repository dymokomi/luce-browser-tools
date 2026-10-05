#!/bin/sh
# Build the libjpeg-turbo ICC oracle against the libjpeg Ladybird's vcpkg tree installs.
cd "$(dirname "$0")"
V=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/vcpkg_installed/arm64-osx-dynamic
clang -O1 -I$V/include libjpeg_oracle.c -L$V/lib -ljpeg -Wl,-rpath,$V/lib -o libjpeg_oracle
