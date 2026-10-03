# Builds float_format_oracle against the reference build's AK (liblagom-ak).
D=/Users/sedov/Dev/luce_dev/.donors/ladybird-pin; B=$D/Build/release; V=$B/vcpkg_installed/arm64-osx-dynamic
cd "$(dirname "$0")"
clang++ -w -std=c++23 -O1 -fno-exceptions -fsigned-char -I$D -I$B -isystem $V/include -isysroot $(xcrun --show-sdk-path) float_format_oracle.cpp -o float_format_oracle -L$B/lib -L$V/lib -llagom-ak -Wl,-rpath,$B/lib -Wl,-rpath,$V/lib
