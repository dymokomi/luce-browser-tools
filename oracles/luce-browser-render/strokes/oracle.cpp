// Oracle for raster's stroker: StrokePath commands drawn as DisplayListPlayerSkia::stroke_path
// draws them in a layer (an SkPaint with kStroke_Style, the width, cap, join, miter limit and an
// SkDashPathEffect, then SkCanvas::drawPath; Skia m144's raster backend), on a white canvas.
//
//   oracle COMMANDS W H OUT.rgba [INDEX]
//
// COMMANDS has one command per line, every float as its IEEE bits in hex:
//   stroke WIDTH CAP JOIN MITER AA DASH_OFFSET N DASH... COLOR | CTM(sx kx tx ky sy ty) | VERB POINTS...
// with raster's verb numbers (0 move, 1 line, 2 quad, 3 cubic, 4 close, 5 conic + weight).
// OUT.rgba gets the unpremultiplied RGBA bytes, row by row. With INDEX, only that command is
// drawn. Besides the pixels, `--path` prints the fill path SkStroke makes for each command
// (skpathutils::FillPathWithPaint with the device's cull rectangle, then the CTM) as bits.
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathUtils.h"
#include "include/core/SkSurface.h"
#include "include/core/SkStrokeRec.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkColorSpace.h"
#include "include/effects/SkDashPathEffect.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static float bits_to_float(const std::string& s) {
    uint32_t u = (uint32_t)std::stoul(s, nullptr, 16);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static uint32_t float_bits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

struct Command {
    SkPaint paint;
    SkMatrix ctm;
    SkPath path;
};

static Command parse(const std::string& line) {
    std::istringstream in(line);
    std::string tok;
    in >> tok; // stroke
    Command c;
    std::string w, cap, join, miter, aa, off, n;
    in >> w >> cap >> join >> miter >> aa >> off >> n;
    std::vector<float> dash;
    for (int i = 0; i < std::stoi(n); i++) {
        in >> tok;
        dash.push_back(bits_to_float(tok));
    }
    std::string color;
    in >> color;
    c.paint.setColor((SkColor)std::stoul(color, nullptr, 16));
    c.paint.setAntiAlias(aa == "true");
    c.paint.setStyle(SkPaint::kStroke_Style);
    c.paint.setStrokeWidth(bits_to_float(w));
    c.paint.setStrokeCap((SkPaint::Cap)std::stoi(cap));
    // raster.LineJoin: miter 0, miter_clip 1, round 2, bevel 3.
    int j = std::stoi(join);
    c.paint.setStrokeJoin(j == 2 ? SkPaint::kRound_Join : j == 3 ? SkPaint::kBevel_Join : SkPaint::kMiter_Join);
    c.paint.setStrokeMiter(bits_to_float(miter));
    c.paint.setPathEffect(SkDashPathEffect::Make(SkSpan<const float>(dash.data(), dash.size()), bits_to_float(off)));
    in >> tok; // |
    float m[6];
    for (float& v : m) {
        in >> tok;
        v = bits_to_float(tok);
    }
    c.ctm = SkMatrix::MakeAll(m[0], m[1], m[2], m[3], m[4], m[5], 0, 0, 1);
    in >> tok; // |
    SkPathBuilder b;
    auto pt = [&]() {
        std::string x, y;
        in >> x >> y;
        return SkPoint::Make(bits_to_float(x), bits_to_float(y));
    };
    while (in >> tok) {
        int verb = std::stoi(tok);
        if (verb == 0) b.moveTo(pt());
        else if (verb == 1) b.lineTo(pt());
        else if (verb == 2) { auto p1 = pt(); b.quadTo(p1, pt()); }
        else if (verb == 3) { auto p1 = pt(); auto p2 = pt(); b.cubicTo(p1, p2, pt()); }
        else if (verb == 4) b.close();
        else if (verb == 5) { auto p1 = pt(); auto p2 = pt(); in >> tok; b.conicTo(p1, p2, bits_to_float(tok)); }
    }
    c.path = b.detach();
    return c;
}

// The raw verbs and points (no auto-close lines), raster's verb numbers.
static void print_path(const SkPath& path) {
    auto pts = path.points();
    auto weights = path.conicWeights();
    size_t p = 0, w = 0;
    auto pt = [&]() { printf(" %08x %08x", float_bits(pts[p].fX), float_bits(pts[p].fY)); p++; };
    for (SkPathVerb verb : path.verbs()) {
        switch (verb) {
            case SkPathVerb::kMove: printf(" 0"); pt(); break;
            case SkPathVerb::kLine: printf(" 1"); pt(); break;
            case SkPathVerb::kQuad: printf(" 2"); pt(); pt(); break;
            case SkPathVerb::kConic: printf(" 5"); pt(); pt(); printf(" %08x", float_bits(weights[w++])); break;
            case SkPathVerb::kCubic: printf(" 3"); pt(); pt(); pt(); break;
            case SkPathVerb::kClose: printf(" 4"); break;
        }
    }
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: oracle COMMANDS W H OUT.rgba [INDEX|--path]\n");
        return 2;
    }
    std::ifstream file(argv[1]);
    std::vector<Command> commands;
    for (std::string line; std::getline(file, line);)
        if (line.rfind("stroke", 0) == 0) commands.push_back(parse(line));
    int w = atoi(argv[2]), h = atoi(argv[3]);
    if (argc > 5 && strcmp(argv[5], "--path") == 0) {
        for (auto& c : commands) {
            // Draw::computeConservativeLocalClipBounds for a device without a clip.
            SkRect cull = SkRect::MakeLTRB(-1, -1, w + 1, h + 1);
            if (auto inverse = c.ctm.invert())
                inverse->mapRect(&cull, cull);
            SkPathBuilder out;
            skpathutils::FillPathWithPaint(c.path, c.paint, &out, &cull, c.ctm);
            out.transform(c.ctm);
            print_path(out.detach());
        }
        return 0;
    }
    if (argc > 5 && strcmp(argv[5], "--dash") == 0) {
        // The dash alone (SkDashImpl::filterPath with the stroke's SkStrokeRec), untransformed.
        for (auto& c : commands) {
            SkRect cull = SkRect::MakeLTRB(-1, -1, w + 1, h + 1);
            if (auto inverse = c.ctm.invert())
                inverse->mapRect(&cull, cull);
            // SkMatrixPriv::ComputeResScaleForStroking
            float sx = SkPoint::Length(c.ctm[SkMatrix::kMScaleX], c.ctm[SkMatrix::kMSkewY]);
            float sy = SkPoint::Length(c.ctm[SkMatrix::kMSkewX], c.ctm[SkMatrix::kMScaleY]);
            float res = std::isfinite(sx) && std::isfinite(sy) && std::max(sx, sy) > 0 ? std::max(sx, sy) : 1;
            SkStrokeRec rec(c.paint, res);
            SkPathBuilder out;
            if (c.paint.getPathEffect() && c.paint.getPathEffect()->filterPath(&out, c.path, &rec, &cull, c.ctm))
                print_path(out.detach());
            else
                printf("\n");
        }
        return 0;
    }
    int only = argc > 5 ? atoi(argv[5]) : -1;
    auto info = SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
    auto surface = SkSurfaces::Raster(info);
    auto* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    // The SVG's paintable draws in a layer (SaveLayer: saveLayer(nullptr, nullptr)).
    canvas->saveLayer(nullptr, nullptr);
    for (size_t i = 0; i < commands.size(); i++) {
        if (only >= 0 && (int)i != only) continue;
        canvas->save();
        canvas->setMatrix(commands[i].ctm);
        canvas->drawPath(commands[i].path, commands[i].paint);
        canvas->restore();
    }
    canvas->restore();
    std::vector<uint8_t> rgba((size_t)w * h * 4);
    auto out_info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, SkColorSpace::MakeSRGB());
    surface->readPixels(out_info, rgba.data(), (size_t)w * 4, 0, 0);
    FILE* f = fopen(argv[4], "wb");
    fwrite(rgba.data(), 1, rgba.size(), f);
    fclose(f);
    return 0;
}
