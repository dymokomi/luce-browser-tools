#include "src/core/SkEdgeClipper.h"
#include "src/core/SkPathPriv.h"
#include "include/core/SkPathBuilder.h"
#include <cstdio>
int main() {
    SkPathBuilder pb;
    pb.moveTo(10, 50);
    pb.cubicTo(0, 175, 195, 70, 75, 20);
    SkPath path = pb.detach();
    auto raw = SkPathPriv::Raw(path, SkResolveConvexity::kYes);
    printf("convex %d\n", raw->isKnownToBeConvex());
    SkRect clip = SkRect::MakeWH(100, 100);
    SkEdgeClipper::ClipPath(*raw, clip, !raw->isKnownToBeConvex(), [](SkEdgeClipper* c, bool, void*) {
        SkPoint pts[4];
        while (auto v = c->next(pts)) {
            int n = *v == SkPathVerb::kLine ? 2 : *v == SkPathVerb::kQuad ? 3 : 4;
            printf("%d:", (int)*v);
            for (int i = 0; i < n; i++) printf(" %.9g,%.9g", pts[i].fX, pts[i].fY);
            printf("\n");
        }
    }, nullptr);
}
