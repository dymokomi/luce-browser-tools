// Oracle for raster's legacy kN32 blitters (SkARGB32_Blitter, _Opaque_, _Black_): scenes drawn
// by Skia m144 into a kN32 (RGBA on macOS and Linux), sRGB, premultiplied raster surface,
// the color type SkBitmapDevice::createDevice gives layers with an image filter, printed as
// premultiplied 0xAARRGGBB words, a row per line (gen_data.py turns them into
// luce-browser-render's src/raster/tests_legacy_blitter_data.lucb). Every scene starts from
// a translucent background so that the blends show; the Luce twins of the scenes are in
// tests_legacy_blitter.lucb and compute every coordinate with the same float operations.
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkSurface.h"
#include <cstdio>
#include <functional>

static const int kSize = 24;

static void scene(const char* name, std::function<void(SkCanvas*)> draw)
{
    auto info = SkImageInfo::Make(kSize, kSize, kN32_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
    auto surface = SkSurfaces::Raster(info);
    auto* c = surface->getCanvas();
    c->clear(SkColorSetARGB(0x60, 0x20, 0x80, 0xc0));
    draw(c);
    SkPixmap pm;
    surface->peekPixels(&pm);
    printf("scene %s\n", name);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            uint32_t p = *pm.addr32(x, y);  // RGBA bytes
            uint32_t r = p & 0xff, g = (p >> 8) & 0xff, b = (p >> 16) & 0xff, a = p >> 24;
            printf("%08x%c", (a << 24) | (r << 16) | (g << 8) | b, x + 1 == kSize ? '\n' : ' ');
        }
    }
}

static SkPaint solid(SkColor color, bool aa)
{
    SkPaint p;
    p.setColor(color);
    p.setAntiAlias(aa);
    return p;
}

// A circle of four quarter conics (SkPathBuilder::addCircle).
static SkPath circle(float cx, float cy, float r)
{
    return SkPathBuilder().addCircle(cx, cy, r).detach();
}

static SkPath star()
{
    return SkPathBuilder()
        .moveTo(12.3f, 1.7f)
        .lineTo(18.6f, 21.2f)
        .lineTo(2.1f, 8.9f)
        .lineTo(22.4f, 8.6f)
        .lineTo(5.8f, 21.9f)
        .close()
        .detach();
}

int main()
{
    scene("circle_translucent", [](SkCanvas* c) { c->drawPath(circle(11.3f, 12.6f, 8.4f), solid(0x5a8a64e5, true)); });
    scene("circle_opaque", [](SkCanvas* c) { c->drawPath(circle(12.5f, 11.2f, 9.1f), solid(0xff20c040, true)); });
    scene("circle_black", [](SkCanvas* c) { c->drawPath(circle(12.1f, 12.4f, 7.7f), solid(0xff000000, true)); });
    scene("star_evenodd", [](SkCanvas* c) {
        SkPath path = star();
        path.setFillType(SkPathFillType::kEvenOdd);
        c->drawPath(path, solid(0xc0e03010, true));
    });
    scene("star_aliased", [](SkCanvas* c) { c->drawPath(star(), solid(0x80102030, false)); });
    scene("rects", [](SkCanvas* c) {
        c->drawRect(SkRect::MakeLTRB(2.3f, 3.6f, 14.7f, 11.2f), solid(0x9040a0f0, true));
        c->drawRect(SkRect::MakeLTRB(8.5f, 9.25f, 21.75f, 20.5f), solid(0xffc08020, true));
        c->drawRect(SkRect::MakeLTRB(1, 15, 7, 22), solid(0x70ff0000, false));
        c->drawRect(SkRect::MakeLTRB(15.2f, 1.4f, 22.9f, 6.6f), solid(0xff000000, true));
    });
    scene("strokes", [](SkCanvas* c) {
        SkPaint p = solid(0xa0305090, true);
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(2.5f);
        c->drawPath(SkPathBuilder().moveTo(2.2f, 3.1f).lineTo(20.4f, 9.7f).quadTo(4.1f, 14.3f, 19.6f, 21.8f).detach(), p);
        p.setColor(0xff000000);
        p.setStrokeWidth(0);
        c->drawPath(SkPathBuilder().moveTo(3.5f, 20.1f).lineTo(21.3f, 2.4f).moveTo(1.2f, 11.5f).lineTo(22.7f, 12.9f).detach(), p);
        p.setColor(0xc0f0d020);
        c->drawPath(SkPathBuilder().moveTo(4.4f, 1.6f).lineTo(6.9f, 22.3f).detach(), p);
    });
    scene("clipped", [](SkCanvas* c) {
        c->clipPath(circle(12, 12, 9.3f), true);
        c->drawRect(SkRect::MakeLTRB(0.5f, 5.5f, 23.5f, 18.5f), solid(0xb0108040, true));
        c->drawPath(star(), solid(0xff000000, true));
    });
    return 0;
}
