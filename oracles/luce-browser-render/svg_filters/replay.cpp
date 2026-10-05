// replay: plays the canvas operations the CPU player logs (save, save_layer, restore,
// clip_rect, effects, fill_rect, fill_path) on Skia m144's raster SkCanvas the way
// DisplayListPlayerSkia makes them, with the image filter of `effects` from a named
// Gfx::Filter graph, and writes the RGBA pixels:
//   replay OPS W H OUT.rgba FILTER
#include <AK/Vector.h>
#include <LibGfx/Filter.h>
#include <LibGfx/FilterImpl.h>
#include <core/SkCanvas.h>
#include <core/SkM44.h>
#include <core/SkColorSpace.h>
#include <core/SkPath.h>
#include <core/SkPathBuilder.h>
#include <core/SkSurface.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

using Gfx::Filter;

static float fbits(std::string const& s)
{
    uint32_t u = (uint32_t)std::stoul(s, nullptr, 16);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static Optional<Filter> make_filter(std::string const& name)
{
    if (name == "offset0")
        return Filter::offset(0, 0);
    if (name == "lbweb") {
        auto flood = Filter::flood(Gfx::Color(0, 0, 0, 255), 0);
        auto shape = Filter::blend(flood, {}, Gfx::CompositingAndBlendingOperator::Normal);
        return Filter::blur(125, 125, shape);
    }
    if (name == "blend") {
        auto flood = Filter::flood(Gfx::Color(0, 0, 0, 255), 0);
        return Filter::blend(flood, {}, Gfx::CompositingAndBlendingOperator::Normal);
    }
    if (name == "blur125")
        return Filter::blur(125, 125);
    return {};
}

int main(int argc, char** argv)
{
    std::ifstream file(argv[1]);
    int w = atoi(argv[2]), h = atoi(argv[3]);
    auto filter = make_filter(argc > 5 ? argv[5] : "");
    auto surface = SkSurfaces::Raster(SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()));
    auto* c = surface->getCanvas();
    for (std::string line; std::getline(file, line);) {
        std::istringstream in(line);
        std::string tag, op;
        in >> tag >> op;
        if (tag != "OP")
            continue;
        if (op == "save") {
            c->save();
        } else if (op == "save_layer") {
            c->saveLayer(nullptr, nullptr);
        } else if (op == "restore") {
            c->restore();
        } else if (op == "clip_rect") {
            float x, y, rw, rh;
            in >> x >> y >> rw >> rh;
            c->clipRect(SkRect::MakeXYWH(x, y, rw, rh), true);
        } else if (op == "translate") {
            float x, y;
            in >> x >> y;
            c->translate(x, y);
        } else if (op == "effects") {
            std::string k;
            float opacity;
            in >> k >> opacity;
            SkPaint paint;
            if (opacity < 1)
                paint.setAlphaf(opacity);
            if (filter.has_value())
                paint.setImageFilter(filter->impl().filter);
            if (getenv("LAYER_BOUNDS")) {
                float l, t, r, b;
                sscanf(getenv("LAYER_BOUNDS"), "%f,%f,%f,%f", &l, &t, &r, &b);
                SkRect bounds = SkRect::MakeLTRB(l, t, r, b);
                c->saveLayer(&bounds, &paint);
            } else {
                c->saveLayer(nullptr, &paint);
            }
            if (getenv("LAYER_CLIP")) {
                float l, t, r, b;
                sscanf(getenv("LAYER_CLIP"), "%f,%f,%f,%f", &l, &t, &r, &b);
                c->clipRect(SkRect::MakeLTRB(l, t, r, b), false);
            }
        } else if (op == "fill_rect") {
            float x, y, rw, rh;
            std::string color;
            in >> x >> y >> rw >> rh >> color;
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor((SkColor)std::stoul(color, nullptr, 16));
            if (getenv("RECT_AS_PATH"))
                c->drawPath(SkPath::Rect(SkRect::MakeXYWH(x, y, rw, rh)), paint);
            else
                c->drawRect(SkRect::MakeXYWH(x, y, rw, rh), paint);
        } else if (op == "m44") {
            // SkM44's column-major storage.
            float m[16];
            for (float& v : m) {
                std::string t;
                in >> t;
                v = fbits(t);
            }
            c->concat(SkM44::ColMajor(m));
        } else if (op == "fill_path" || op == "clip_path") {
            std::string color, aa, rule;
            if (op == "fill_path")
                in >> color >> aa >> rule;
            else
                in >> rule;
            SkPathBuilder b;
            std::string t;
            auto pt = [&]() { std::string x, y; in >> x >> y; return SkPoint::Make(fbits(x), fbits(y)); };
            while (in >> t) {
                int v = std::stoi(t);
                if (v == 0) b.moveTo(pt());
                else if (v == 1) b.lineTo(pt());
                else if (v == 2) { auto p1 = pt(); b.quadTo(p1, pt()); }
                else if (v == 3) { auto p1 = pt(); auto p2 = pt(); b.cubicTo(p1, p2, pt()); }
                else if (v == 4) b.close();
                else if (v == 5) { auto p1 = pt(); auto p2 = pt(); std::string wt; in >> wt; b.conicTo(p1, p2, fbits(wt)); }
            }
            b.setFillType(rule == "1" ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding);
            if (op == "clip_path") {
                c->clipPath(b.detach(), true);
                continue;
            }
            SkPaint paint;
            paint.setColor((SkColor)std::stoul(color, nullptr, 16));
            paint.setAntiAlias(aa == "true");
            c->drawPath(b.detach(), paint);
        }
    }
    std::vector<uint8_t> rgba((size_t)w * h * 4);
    surface->readPixels(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, SkColorSpace::MakeSRGB()), rgba.data(), (size_t)w * 4, 0, 0);
    FILE* f = fopen(argv[4], "wb");
    fwrite(rgba.data(), 1, rgba.size(), f);
    fclose(f);
    return 0;
}
