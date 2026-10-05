#!/bin/sh
# Builds the image_codecs oracle (region p2img): ladybird_frames.cpp linked with the reference build's lagom libraries.
R=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
B=$R/Build/release
cd "$(dirname "$0")"
LIBS=$(ls $B/lib/liblagom-*.0.1.0.dylib | grep -v liblagom-web.0)
/usr/bin/clang++ -I$R -I$R/Libraries -I$R/Services -I$B -I$B/Libraries -I$B/Services -I$B/vcpkg_installed/arm64-osx-dynamic/include -O1 -std=c++23 -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk -mmacosx-version-min=14.0 -fno-exceptions -fsigned-char -Wno-user-defined-literals -Wno-c23-extensions -Wno-unqualified-std-cast-call -Wno-unknown-warning-option -Wno-unknown-pragmas -Wno-keyword-macro -Wno-invalid-offsetof ladybird_frames.cpp $LIBS -L$B/vcpkg_installed/arm64-osx-dynamic/lib -Wl,-rpath,$B/vcpkg_installed/arm64-osx-dynamic/lib -Wl,-rpath,$B/lib -o ladybird_frames
