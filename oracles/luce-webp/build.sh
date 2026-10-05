#!/bin/sh
# Builds libwebp 1.6.0 (the version Ladybird links) from the donor's vcpkg source tree twice, with
# SIMD (as Ladybird ships it) and without (the scalar reference for speed), static, into build/,
# then the oracle over each: webp_oracle (SIMD) and webp_oracle_scalar.
set -e
cd "$(dirname "$0")"
SRC=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/vcpkg/buildtrees/libwebp/src/v1.6.0-7e52d1be79.clean
COMMON="-G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF -DWEBP_BUILD_VWEBP=OFF -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_WEBPMUX=OFF -DWEBP_BUILD_EXTRAS=OFF"
for kind in simd scalar; do
    flag=ON
    [ $kind = scalar ] && flag=OFF
    if [ ! -f build/$kind/libwebp.a ]; then
        cmake -S $SRC -B build/$kind $COMMON -DWEBP_ENABLE_SIMD=$flag > build-$kind.log
        ninja -C build/$kind webp webpdemux libwebpmux >> build-$kind.log
    fi
    out=webp_oracle
    [ $kind = scalar ] && out=webp_oracle_scalar
    clang -O2 -I$SRC/src -I$SRC webp_oracle.c build/$kind/libwebpdemux.a build/$kind/libwebpmux.a build/$kind/libwebp.a build/$kind/libsharpyuv.a -lpthread -o $out
done
