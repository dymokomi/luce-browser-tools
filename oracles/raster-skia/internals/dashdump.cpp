#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathUtils.h"
#include "include/core/SkRect.h"
#include "include/effects/SkDashPathEffect.h"
#include <cstdio>
#include "include/core/SkStrokeRec.h"
#include <cstring>
static unsigned B(float f){unsigned u;memcpy(&u,&f,4);return u;}
static void dump(const SkPath& p) {
    SkPath::Iter it(p, false);
    SkPoint pts[4];
    SkPath::Verb v;
    while ((v = it.next(pts)) != SkPath::kDone_Verb) {
        int n = 0; const char* c = "";
        switch (v) {
            case SkPath::kMove_Verb: c="M"; n=1; break;
            case SkPath::kLine_Verb: c="L"; n=2; break;
            case SkPath::kQuad_Verb: c="Q"; n=3; break;
            case SkPath::kConic_Verb: c="K"; n=3; break;
            case SkPath::kCubic_Verb: c="C"; n=4; break;
            case SkPath::kClose_Verb: c="Z"; n=0; break;
            default: break;
        }
        printf("%s\n", c);
        for (int i = (v == SkPath::kMove_Verb ? 0 : 1); i < n; i++) printf("  %u %u\n", B(pts[i].fX), B(pts[i].fY));
    }
}
int main(int argc, char** argv) {
    SkPathBuilder pb;
    bool multi = argc > 2;
    if (multi) {
    pb.moveTo(49.0f, 76.0f);
    pb.cubicTo(22.0f, 150.0f, 11.0f, 213.0f, 186.0f, 151.0f);
    pb.cubicTo(194.0f, 106.0f, 195.0f, 64.0f, 169.0f, 26.0f);
    pb.moveTo(124.0f, 41.0f);
    pb.lineTo(162.0f, 105.0f);
    pb.cubicTo(135.0f, 175.0f, 97.0f, 166.0f, 53.0f, 128.0f);
    pb.lineTo(93.0f, 71.0f);
    pb.moveTo(24.0f, 52.0f);
    pb.lineTo(108.0f, 20.0f);
    } else {
    pb.moveTo(28.7f, 23.9f);
    pb.lineTo(177.4f, 35.2f);
    pb.lineTo(177.4f, 68.0f);
    pb.lineTo(129.7f, 68.0f);
    pb.cubicTo(81.6f, 59.3f, 41.8f, 63.3f, 33.4f, 115.2f);
    pb.cubicTo(56.8f, 128.7f, 77.3f, 143.8f, 53.3f, 183.8f);
    pb.cubicTo(113.8f, 185.7f, 91.0f, 109.7f, 167.3f, 111.8f);
    pb.cubicTo(-56.2f, 90.3f, 177.3f, 68.0f, 110.2f, 95.5f);
    }
    SkPath path = pb.detach();
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2);
    float iv[2] = {10, 5};
    paint.setPathEffect(SkDashPathEffect::Make(SkSpan<const float>(iv, 2), 2));
    SkPathBuilder out;
    SkRect cull = SkRect::MakeLTRB(-1, -1, 201, 201);
    bool mode = argc > 1 && argv[1][0] == 'd';
    if (mode) {
        // dash only
        SkStrokeRec rec(paint);
        paint.getPathEffect()->filterPath(&out, path, &rec, &cull, SkMatrix::I());
    } else {
        skpathutils::FillPathWithPaint(path, paint, &out, &cull, SkMatrix::I());
    }
    dump(out.detach());
}
