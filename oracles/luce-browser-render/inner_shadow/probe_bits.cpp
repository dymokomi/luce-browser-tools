// The bits of SkPathOps' difference for the shadow_inner_rrect scene.
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/pathops/SkPathOps.h"
#include <cstdio>
#include <cstring>
static unsigned b(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static SkRRect rr(SkRect r, float a, float c, float d, float e)
{
    SkVector radii[4] = { { a, a }, { c, c }, { d, d }, { e, e } };
    SkRRect out;
    out.setRectRadii(r, radii);
    return out;
}
int main()
{
    SkPath o = SkPath::RRect(rr(SkRect::MakeXYWH(0, 0, 40, 32), 6, 4, 8, 3)), i = SkPath::RRect(rr(SkRect::MakeXYWH(10, 9, 22, 16), 3, 2, 5, 1)), r;
    Op(o, i, kDifference_SkPathOp, &r);
    printf("fill %d\n", (int)r.getFillType());
    auto pts = r.points();
    auto w = r.conicWeights();
    size_t p = 0, k = 0;
    for (auto v : r.verbs()) {
        int n = v == SkPathVerb::kMove || v == SkPathVerb::kLine ? 1 : v == SkPathVerb::kClose ? 0 : v == SkPathVerb::kCubic ? 3 : 2;
        printf("%d", (int)v);
        for (int j = 0; j < n; j++, p++) printf(" %08x %08x", b(pts[p].fX), b(pts[p].fY));
        if (v == SkPathVerb::kConic) printf(" w%08x", b(w[k++]));
        printf("\n");
    }
}
