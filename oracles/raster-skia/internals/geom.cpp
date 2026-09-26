#include "src/core/SkGeometry.h"
#include <cstdio>
int main() {
    SkPoint src[4] = {{10, 20}, {67, 437}, {298, 213}, {401, 214}};
    SkPoint dst[10];
    int n = SkChopCubicAtYExtrema(src, dst);
    printf("n=%d\n", n);
    for (int i = 0; i < 10; i++) printf("%.9g %.9g\n", dst[i].fX, dst[i].fY);
}
