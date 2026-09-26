#!/bin/sh
# Builds the oracle against the reference build (Entities.cpp and the generated tables are
# compiled in because liblagom-web does not export them).
R=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin
cd "$(dirname "$0")"
/usr/bin/clang++ -I$R -I$R/Libraries -I$R/Services -I$R/Build/release -I$R/Build/release/Libraries -I$R/Build/release/Services -O1 -std=c++23 -arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk -mmacosx-version-min=14.0 -fno-exceptions -fsigned-char -Wno-user-defined-literals -Wno-c23-extensions -Wno-unqualified-std-cast-call -Wno-unknown-warning-option -Wno-unknown-pragmas oracle.cpp $R/Libraries/LibWeb/HTML/Parser/Entities.cpp $R/Build/release/Libraries/LibWeb/HTML/Parser/NamedCharacterReferences.cpp -o oracle -Wl,-rpath,$R/Build/release/vcpkg_installed/arm64-osx-dynamic/lib -Wl,-rpath,$R/Build/release/lib $R/Build/release/lib/liblagom-web.0.1.0.dylib $R/Build/release/lib/liblagom-ak.0.1.0.dylib $R/Build/release/lib/liblagom-core.0.1.0.dylib
