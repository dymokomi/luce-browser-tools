// Probe: what SkPathOps' kDifference makes of two rounded rectangles (paint_inner_box_shadow).
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/pathops/SkPathOps.h"
#include <cstdio>
static void dump(const char* name, const SkPath& p)
{
    printf("%s fill %d:", name, (int)p.getFillType());
    SkPath::Iter it(p, false);
    while (auto rec = it.next()) {
        auto pts = rec->fPoints;
        switch (rec->fVerb) {
        case SkPathVerb::kMove: printf("\n  M %g %g", pts[0].fX, pts[0].fY); break;
        case SkPathVerb::kLine: printf(" L %g %g", pts[1].fX, pts[1].fY); break;
        case SkPathVerb::kQuad: printf(" Q %g %g %g %g", pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY); break;
        case SkPathVerb::kConic: printf(" C %g %g %g %g w%g", pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY, rec->conicWeight()); break;
        case SkPathVerb::kCubic: printf(" B %g %g %g %g %g %g", pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY, pts[3].fX, pts[3].fY); break;
        case SkPathVerb::kClose: printf(" Z"); break;
        }
    }
    printf("\n");
}
static void op(const char* name, SkRRect outer, SkRRect inner)
{
    SkPath o = SkPath::RRect(outer), i = SkPath::RRect(inner), r;
    if (!Op(o, i, kDifference_SkPathOp, &r)) { printf("%s failed\n", name); return; }
    dump(name, r);
}
int main()
{
    op("rects", SkRRect::MakeRect(SkRect::MakeLTRB(10, 10, 60, 40)), SkRRect::MakeRect(SkRect::MakeLTRB(15, 14, 50, 35)));
    op("rrects", SkRRect::MakeRectXY(SkRect::MakeLTRB(10, 10, 60, 40), 8, 8), SkRRect::MakeRectXY(SkRect::MakeLTRB(15, 14, 50, 35), 4, 4));
    op("rrect_in_rect", SkRRect::MakeRect(SkRect::MakeLTRB(10, 10, 60, 40)), SkRRect::MakeRectXY(SkRect::MakeLTRB(15, 14, 50, 35), 4, 4));
    op("overlap", SkRRect::MakeRectXY(SkRect::MakeLTRB(10, 10, 60, 40), 8, 8), SkRRect::MakeRectXY(SkRect::MakeLTRB(20, 20, 70, 50), 8, 8));
}
