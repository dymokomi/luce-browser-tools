// Oracle for the raster module's perspective matrices and paths: SkMatrix m144's concatenation,
// inversion, point mapping, normalizePerspective and mapRect with a perspective row, and
// SkPath::transform / SkPathBuilder::transform of paths that cross the w = 0 plane, printed
// as the bits of each float (raster's tests_perspective.lucb compares them bit for bit).
#include "include/core/SkMatrix.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkRect.h"
#include <cstdio>
#include <cstring>

static unsigned bits(float f)
{
    unsigned u;
    memcpy(&u, &f, 4);
    return u;
}

static void print_matrix(char const* name, SkMatrix const& m)
{
    printf("%s", name);
    for (int i = 0; i < 9; ++i)
        printf(" %08x", bits(m[i]));
    printf("\n");
}

static void print_path(char const* name, SkPath const& path)
{
    printf("%s verbs", name);
    for (auto verb : path.verbs())
        printf(" %d", int(verb));
    printf("\n%s points", name);
    for (auto p : path.points())
        printf(" %08x %08x", bits(p.fX), bits(p.fY));
    printf("\n%s weights", name);
    for (auto w : path.conicWeights())
        printf(" %08x", bits(w));
    printf("\n");
}

int main()
{
    SkMatrix a = SkMatrix::MakeAll(0.9f, 0.3f, 12, -0.2f, 1.1f, -7, 0.004f, -0.0025f, 1.2f);
    SkMatrix b = SkMatrix::MakeAll(1.5f, 0, 3, 0, 0.75f, -2, 0, 0, 1);
    SkMatrix c = SkMatrix::MakeAll(0.70710677f, -0.70710677f, 5, 0.70710677f, 0.70710677f, 1, 0, 0, 1);
    print_matrix("concat_ab", SkMatrix::Concat(a, b));
    print_matrix("concat_ba", SkMatrix::Concat(b, a));
    print_matrix("concat_ac", SkMatrix::Concat(a, c));
    print_matrix("concat_aa", SkMatrix::Concat(a, a));
    print_matrix("invert_a", *a.invert());
    SkMatrix n = SkMatrix::MakeAll(2, 0, 4, 0, 2, 6, 0, 0, 2);
    n.normalizePerspective();
    print_matrix("normalize", n);

    SkPoint pts[3] = { { 0, 0 }, { 10, -4 }, { 100, 50 } };
    a.mapPoints(pts);
    printf("map_points");
    for (auto p : pts)
        printf(" %08x %08x", bits(p.fX), bits(p.fY));
    printf("\n");

    // w = 0.02 x + 0.5 crosses zero at x = -25.
    SkMatrix p = SkMatrix::MakeAll(1, 0.25f, 3, 0.1f, 1, 2, 0.02f, 0.001f, 0.5f);
    SkRect r = p.mapRect(SkRect::MakeLTRB(-40, -20, 40, 20));
    printf("map_rect %08x %08x %08x %08x\n", bits(r.fLeft), bits(r.fTop), bits(r.fRight), bits(r.fBottom));

    SkPathBuilder builder;
    builder.moveTo(-40, -10);
    builder.cubicTo(-40, 30, 40, 30, 40, -10);
    builder.quadTo(0, -40, -40, -10);
    builder.close();
    builder.moveTo(-30, -5);
    builder.lineTo(30, -5);
    builder.conicTo(36, 0, 30, 5, 0.6f);
    builder.lineTo(-30, 5);
    SkPath path = builder.detach();

    SkPath through_iter;
    path.transform(p, &through_iter);
    print_path("path_iter", through_iter);

    SkPathBuilder raw(path);
    raw.transform(p);
    print_path("path_raw", raw.detach());

    // All in front of the plane: no cut.
    SkMatrix q = SkMatrix::MakeAll(1, 0, 0, 0, 1, 0, 0.001f, 0.002f, 1);
    SkPath front;
    path.transform(q, &front);
    print_path("path_front", front);
    return 0;
}
