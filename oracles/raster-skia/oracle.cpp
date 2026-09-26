// oracle.cpp - the helpers of oracle.h and the driver: `oracle OUT_DIR [scene ...]`.
#include "oracle.h"

#include "include/core/SkRegion.h"
#include <zlib.h>
#include <sys/stat.h>
#include <cstring>

std::string g_out_dir = "out";

std::vector<Scene>& scenes() {
    static std::vector<Scene> all;
    return all;
}

// mark: pixmaps ------------------------------------------------------------------------------

SkImageInfo ladybird_info(int w, int h) {
    return SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
}

Pixmap new_pixmap(int w, int h) {
    Pixmap pm;
    pm.bitmap.allocPixels(ladybird_info(w, h));
    pm.bitmap.eraseColor(SK_ColorTRANSPARENT);
    return pm;
}

static uint8_t to_u8(float v) { return (uint8_t)(v * 255.0f + 0.5f); }

void pixmap_fill(Pixmap& pm, SkColor4f c) {
    // tiny-skia: color.premultiply().to_color_u8(), stored in every pixel.
    float a = c.fA;
    uint8_t r = to_u8(c.fR * a), g = to_u8(c.fG * a), b = to_u8(c.fB * a), aa = to_u8(a);
    for (int y = 0; y < pm.height(); y++)
        for (int x = 0; x < pm.width(); x++) {
            uint8_t* p = (uint8_t*)pm.bitmap.getAddr32(x, y);
            p[0] = b; p[1] = g; p[2] = r; p[3] = aa;
        }
}

std::vector<uint8_t> pixmap_rgba(const Pixmap& pm) {
    std::vector<uint8_t> out((size_t)pm.width() * pm.height() * 4);
    for (int y = 0; y < pm.height(); y++)
        for (int x = 0; x < pm.width(); x++) pm.rgba(x, y, &out[((size_t)y * pm.width() + x) * 4]);
    return out;
}

Pixmap pixmap_from_rgba(int w, int h, const std::vector<uint8_t>& rgba) {
    Pixmap pm = new_pixmap(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const uint8_t* s = &rgba[((size_t)y * w + x) * 4];
            uint8_t* p = (uint8_t*)pm.bitmap.getAddr32(x, y);
            p[0] = s[2]; p[1] = s[1]; p[2] = s[0]; p[3] = s[3];
        }
    return pm;
}

static void make_dirs(const std::string& path) {
    for (size_t i = 1; i < path.size(); i++)
        if (path[i] == '/') mkdir(path.substr(0, i).c_str(), 0755);
}

static void put_u32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

static void chunk(std::vector<uint8_t>& png, const char* type, const std::vector<uint8_t>& data) {
    put_u32(png, (uint32_t)data.size());
    size_t start = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    uLong crc = crc32(0, png.data() + start, (uInt)(png.size() - start));
    put_u32(png, (uint32_t)crc);
}

static void write_png(const std::string& path, int w, int h, int channels, const std::vector<uint8_t>& pixels) {
    std::vector<uint8_t> raw;
    for (int y = 0; y < h; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), pixels.begin() + (size_t)y * w * channels, pixels.begin() + (size_t)(y + 1) * w * channels);
    }
    uLongf len = compressBound(raw.size());
    std::vector<uint8_t> z(len);
    compress2(z.data(), &len, raw.data(), raw.size(), 9);
    z.resize(len);
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    std::vector<uint8_t> ihdr;
    put_u32(ihdr, w); put_u32(ihdr, h);
    ihdr.push_back(8); ihdr.push_back(channels == 4 ? 6 : 0); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
    chunk(png, "IHDR", ihdr);
    chunk(png, "IDAT", z);
    chunk(png, "IEND", {});
    make_dirs(path);
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path.c_str()); exit(2); }
    fwrite(png.data(), 1, png.size(), f);
    fclose(f);
}

void save(const Pixmap& pm, const std::string& name) {
    std::vector<uint8_t> premul = pixmap_rgba(pm);
    std::vector<uint8_t> demul = premul;
    for (size_t i = 0; i < demul.size(); i += 4) {
        uint8_t a = demul[i + 3];
        if (a == 255) continue;
        if (a == 0) { demul[i] = demul[i + 1] = demul[i + 2] = 0; continue; }
        double af = a / 255.0;
        for (int c = 0; c < 3; c++) {
            double v = demul[i + c] / af + 0.5;
            demul[i + c] = v >= 255.0 ? 255 : (uint8_t)v;
        }
    }
    std::string path = g_out_dir + "/" + name;
    write_png(path, pm.width(), pm.height(), 4, demul);
    std::string raw = path + ".rgba";
    FILE* f = fopen(raw.c_str(), "wb");
    uint32_t wh[2] = {(uint32_t)pm.width(), (uint32_t)pm.height()};
    fwrite(wh, 4, 2, f);
    fwrite(premul.data(), 1, premul.size(), f);
    fclose(f);
}

void save_mask(const std::vector<uint8_t>& mask, int w, int h, const std::string& name) {
    write_png(g_out_dir + "/" + name, w, h, 1, mask);
}

// mark: paths --------------------------------------------------------------------------------

void PathBuilder::inject_move() {
    if (!has_move) {
        // tiny-skia: after a close, the next segment starts at the last move_to point.
        move_to(last_move.fX, last_move.fY);
    }
}

void PathBuilder::move_to(float x, float y) {
    if (last_is_move) {
        // Consecutive move_tos: the last one replaces the previous.
        SkPath p = b.snapshot();
        SkPathBuilder nb;
        // rebuild without the trailing move
        SkPath::Iter it(p, false);
        SkPoint pts[4];
        std::vector<std::pair<SkPath::Verb, std::vector<SkPoint>>> segs;
        std::vector<float> weights; // one per conic, in order
        SkPath::Verb v;
        while ((v = it.next(pts)) != SkPath::kDone_Verb) {
            int n = v == SkPath::kMove_Verb ? 1 : v == SkPath::kLine_Verb ? 2 : v == SkPath::kQuad_Verb || v == SkPath::kConic_Verb ? 3 : v == SkPath::kCubic_Verb ? 4 : 0;
            if (v == SkPath::kConic_Verb) weights.push_back(it.conicWeight());
            segs.push_back({v, std::vector<SkPoint>(pts, pts + n)});
        }
        size_t conic = 0;
        // SkPath::Iter drops a trailing move; so just replay everything
        for (auto& s : segs) {
            switch (s.first) {
                case SkPath::kMove_Verb: nb.moveTo(s.second[0]); break;
                case SkPath::kLine_Verb: nb.lineTo(s.second[1]); break;
                case SkPath::kQuad_Verb: nb.quadTo(s.second[1], s.second[2]); break;
                case SkPath::kConic_Verb: nb.conicTo(s.second[1], s.second[2], weights[conic++]); break;
                case SkPath::kCubic_Verb: nb.cubicTo(s.second[1], s.second[2], s.second[3]); break;
                case SkPath::kClose_Verb: nb.close(); break;
                default: break;
            }
        }
        b = nb;
        verbs--;
    }
    b.moveTo(x, y);
    verbs++;
    has_move = true;
    last_is_move = true;
    last_move = last = {x, y};
}

void PathBuilder::line_to(float x, float y) {
    inject_move();
    b.lineTo(x, y);
    verbs++;
    last_is_move = false;
    last = {x, y};
}

void PathBuilder::quad_to(float x1, float y1, float x, float y) {
    inject_move();
    b.quadTo(x1, y1, x, y);
    verbs++;
    last_is_move = false;
    last = {x, y};
}

void PathBuilder::cubic_to(float x1, float y1, float x2, float y2, float x, float y) {
    inject_move();
    b.cubicTo(x1, y1, x2, y2, x, y);
    verbs++;
    last_is_move = false;
    last = {x, y};
}

void PathBuilder::conic_to(float x1, float y1, float x, float y, float w) {
    inject_move();
    b.conicTo(x1, y1, x, y, w);
    verbs++;
    last_is_move = false;
    last = {x, y};
}

void PathBuilder::close() {
    if (verbs > 0 && !last_is_move) {
        b.close();
        verbs++;
    }
    has_move = false;
    last_is_move = false;
}

void PathBuilder::push_rect(SkRect r) {
    move_to(r.left(), r.top());
    line_to(r.right(), r.top());
    line_to(r.right(), r.bottom());
    line_to(r.left(), r.bottom());
    close();
}

void PathBuilder::push_oval(SkRect oval) {
    float cx = oval.left() * 0.5f + oval.right() * 0.5f;
    float cy = oval.top() * 0.5f + oval.bottom() * 0.5f;
    SkPoint op[4] = {{cx, oval.bottom()}, {oval.left(), cy}, {cx, oval.top()}, {oval.right(), cy}};
    SkPoint rp[4] = {{oval.right(), oval.bottom()}, {oval.left(), oval.bottom()}, {oval.left(), oval.top()}, {oval.right(), oval.top()}};
    const float weight = 0.707106781f; // SCALAR_ROOT_2_OVER_2
    move_to(op[3].fX, op[3].fY);
    for (int i = 0; i < 4; i++) conic_to(rp[i].fX, rp[i].fY, op[i].fX, op[i].fY, weight);
    close();
}

void PathBuilder::push_circle(float x, float y, float r) {
    SkRect rect = SkRect::MakeXYWH(x - r, y - r, r + r, r + r);
    if (rect.width() > 0 && rect.height() > 0) push_oval(rect);
}

std::optional<SkPath> PathBuilder::finish() {
    if (verbs <= 1) return std::nullopt;
    SkPath p = b.detach();
    if (!p.isFinite()) return std::nullopt;
    return p;
}

SkPath path_from_rect(SkRect r) {
    PathBuilder pb;
    pb.push_rect(r);
    return *pb.finish();
}

std::optional<SkPath> path_from_circle(float cx, float cy, float r) {
    PathBuilder pb;
    pb.push_circle(cx, cy, r);
    return pb.finish();
}

std::optional<SkPath> path_from_oval(SkRect r) {
    PathBuilder pb;
    pb.push_oval(r);
    return pb.finish();
}

// mark: paints -------------------------------------------------------------------------------

SkPaint Paint::to_sk() const {
    SkPaint p;
    if (shader) {
        p.setShader(shader);
        p.setAlphaf(shader_opacity);
    } else {
        p.setColor4f(color);
    }
    p.setBlendMode(blend_mode);
    p.setAntiAlias(anti_alias);
    p.setDither(dither);
    return p;
}

static SkTileMode tile(SpreadMode m) {
    switch (m) {
        case SpreadMode::Pad: return SkTileMode::kClamp;
        case SpreadMode::Reflect: return SkTileMode::kMirror;
        case SpreadMode::Repeat: return SkTileMode::kRepeat;
    }
    return SkTileMode::kClamp;
}

static void split(const std::vector<GradientStop>& stops, std::vector<SkColor4f>& colors, std::vector<float>& pos) {
    for (auto& s : stops) { colors.push_back(s.color); pos.push_back(s.pos); }
}

sk_sp<SkShader> linear_gradient(SkPoint start, SkPoint end, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts) {
    std::vector<SkColor4f> colors; std::vector<float> pos; split(stops, colors, pos);
    SkPoint pts[2] = {start, end};
    return SkGradientShader::MakeLinear(pts, colors.data(), nullptr, pos.data(), (int)colors.size(), tile(mode), 0, &ts);
}

sk_sp<SkShader> radial_gradient(SkPoint start, float start_radius, SkPoint end, float end_radius, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts) {
    std::vector<SkColor4f> colors; std::vector<float> pos; split(stops, colors, pos);
    return SkGradientShader::MakeTwoPointConical(start, start_radius, end, end_radius, colors.data(), nullptr, pos.data(), (int)colors.size(), tile(mode), 0, &ts);
}

sk_sp<SkShader> sweep_gradient(SkPoint center, float start_angle, float end_angle, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts) {
    std::vector<SkColor4f> colors; std::vector<float> pos; split(stops, colors, pos);
    return SkGradientShader::MakeSweep(center.fX, center.fY, colors.data(), nullptr, pos.data(), (int)colors.size(), tile(mode), start_angle, end_angle, 0, &ts);
}

SkSamplingOptions sampling(FilterQuality q) {
    switch (q) {
        case FilterQuality::Nearest: return SkSamplingOptions(SkFilterMode::kNearest);
        case FilterQuality::Bilinear: return SkSamplingOptions(SkFilterMode::kLinear);
        case FilterQuality::Bicubic: return SkSamplingOptions(SkCubicResampler::Mitchell());
    }
    return {};
}

sk_sp<SkShader> pattern(const Pixmap& pm, SpreadMode mode, FilterQuality q, const SkMatrix& ts) {
    return pm.image()->makeShader(tile(mode), tile(mode), sampling(q), &ts);
}

std::optional<std::pair<std::vector<float>, float>> stroke_dash(std::vector<float> intervals, float offset) {
    if (!std::isfinite(offset)) return std::nullopt;
    if (intervals.size() < 2 || intervals.size() % 2 != 0) return std::nullopt;
    float sum = 0.0f;
    for (float v : intervals) {
        if (v < 0.0f) return std::nullopt;
        sum += v;
    }
    if (!(sum > 0.0f) || !std::isfinite(sum)) return std::nullopt;
    return std::make_pair(std::move(intervals), offset);
}

void apply_opacity(std::vector<GradientStop>& stops, float opacity) {
    float o = opacity < 0.0f ? 0.0f : opacity > 1.0f ? 1.0f : opacity;
    for (auto& s : stops) s.color.fA *= o;
}

// mark: masks --------------------------------------------------------------------------------

void Mask::fill_path(const SkPath& path, FillRule rule, bool aa, const SkMatrix& ts) {
    ops.clear();
    intersect_path(path, rule, aa, ts);
}

void Mask::intersect_path(const SkPath& path, FillRule rule, bool aa, const SkMatrix& ts) {
    SkPath p = path.makeTransform(ts);
    p.setFillType(rule == FillRule::Winding ? SkPathFillType::kWinding : SkPathFillType::kEvenOdd);
    ops.push_back({p, aa});
}

void Mask::apply(SkCanvas& canvas) const {
    canvas.clipRect(SkRect::MakeWH(w, h));
    for (auto& op : ops) canvas.clipPath(op.path, SkClipOp::kIntersect, op.aa);
}

std::vector<uint8_t> Mask::coverage() const {
    SkBitmap bm;
    bm.allocPixels(SkImageInfo::MakeA8(w, h));
    bm.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas canvas(bm);
    apply(canvas);
    SkPaint p;
    p.setColor(SK_ColorWHITE);
    canvas.drawPaint(p);
    std::vector<uint8_t> out((size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) out[(size_t)y * w + x] = *bm.getAddr8(x, y);
    return out;
}

std::vector<uint8_t> mask_from_pixmap(const Pixmap& pm, MaskType type) {
    std::vector<uint8_t> out((size_t)pm.width() * pm.height());
    for (int y = 0; y < pm.height(); y++)
        for (int x = 0; x < pm.width(); x++) {
            uint8_t p[4];
            pm.rgba(x, y, p);
            uint8_t& ma = out[(size_t)y * pm.width() + x];
            if (type == MaskType::Alpha) { ma = p[3]; continue; }
            float r = (float)p[0] / 255.0f, g = (float)p[1] / 255.0f, b = (float)p[2] / 255.0f;
            float a = (float)p[3] / 255.0f;
            if (p[3] != 0) { r /= a; g /= a; b /= a; }
            float luma = r * 0.2126f + g * 0.7152f + b * 0.0722f;
            float v = (luma * a) * 255.0f;
            v = v < 0.0f ? 0.0f : v > 255.0f ? 255.0f : v;
            ma = (uint8_t)std::ceil(v);
        }
    return out;
}

// Pixmap::apply_mask as Skia does it: the mask as an A8 image drawn with DstIn.
void apply_mask(Pixmap& pm, const std::vector<uint8_t>& mask) {
    SkImageInfo info = SkImageInfo::MakeA8(pm.width(), pm.height());
    SkPixmap src(info, mask.data(), pm.width());
    sk_sp<SkImage> img = SkImages::RasterFromPixmapCopy(src);
    SkCanvas canvas = pm.canvas();
    SkPaint p;
    p.setBlendMode(SkBlendMode::kDstIn);
    canvas.drawImage(img, 0, 0, SkSamplingOptions(), &p);
}

std::optional<Pixmap> clone_rect(const Pixmap& pm, int x, int y, int w, int h) {
    SkIRect r = SkIRect::MakeXYWH(x, y, w, h);
    if (!r.intersect(SkIRect::MakeWH(pm.width(), pm.height()))) return std::nullopt;
    Pixmap out = new_pixmap(r.width(), r.height());
    for (int j = 0; j < r.height(); j++)
        for (int i = 0; i < r.width(); i++)
            *out.bitmap.getAddr32(i, j) = *pm.bitmap.getAddr32(r.left() + i, r.top() + j);
    return out;
}

static uint8_t premultiply_u8(int c, int a) {
    int prod = c * a + 128;
    return (uint8_t)((prod + (prod >> 8)) >> 8);
}

void check_pixels(const Pixmap& pm, const std::vector<std::array<int, 4>>& expected, const char* what) {
    int bad = 0;
    for (int i = 0; i < (int)expected.size(); i++) {
        auto& e = expected[i];
        uint8_t want[4] = {premultiply_u8(e[0], e[3]), premultiply_u8(e[1], e[3]), premultiply_u8(e[2], e[3]), (uint8_t)e[3]};
        uint8_t got[4];
        pm.rgba(i % pm.width(), i / pm.width(), got);
        if (memcmp(want, got, 4) != 0) {
            fprintf(stderr, "%s: pixel %d,%d is %d,%d,%d,%d, tiny-skia expects %d,%d,%d,%d\n", what, i % pm.width(), i / pm.width(),
                    got[0], got[1], got[2], got[3], want[0], want[1], want[2], want[3]);
            bad++;
        }
    }
    if (!bad) fprintf(stderr, "%s: pixels match tiny-skia's expected array\n", what);
}

// mark: drawing ------------------------------------------------------------------------------

void fill_path(Pixmap& pm, const SkPath& path0, const Paint& paint, FillRule rule, const SkMatrix& ts, const Mask* mask) {
    SkCanvas canvas = pm.canvas();
    if (mask) mask->apply(canvas);
    canvas.concat(ts);
    SkPath path = path0;
    path.setFillType(rule == FillRule::Winding ? SkPathFillType::kWinding : SkPathFillType::kEvenOdd);
    canvas.drawPath(path, paint.to_sk());
}

void fill_rect(Pixmap& pm, SkRect rect, const Paint& paint, const SkMatrix& ts, const Mask* mask) {
    SkCanvas canvas = pm.canvas();
    if (mask) mask->apply(canvas);
    canvas.concat(ts);
    canvas.drawRect(rect, paint.to_sk());
}

void stroke_path(Pixmap& pm, const SkPath& path, const Paint& paint, const Stroke& stroke, const SkMatrix& ts, const Mask* mask) {
    SkCanvas canvas = pm.canvas();
    if (mask) mask->apply(canvas);
    canvas.concat(ts);
    SkPaint p = paint.to_sk();
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(stroke.width);
    p.setStrokeMiter(stroke.miter_limit);
    switch (stroke.line_cap) {
        case LineCap::Butt: p.setStrokeCap(SkPaint::kButt_Cap); break;
        case LineCap::Round: p.setStrokeCap(SkPaint::kRound_Cap); break;
        case LineCap::Square: p.setStrokeCap(SkPaint::kSquare_Cap); break;
    }
    switch (stroke.line_join) {
        case LineJoin::Miter: p.setStrokeJoin(SkPaint::kMiter_Join); break;
        case LineJoin::MiterClip: p.setStrokeJoin(SkPaint::kMiter_Join); break; // Skia has no miter-clip
        case LineJoin::Round: p.setStrokeJoin(SkPaint::kRound_Join); break;
        case LineJoin::Bevel: p.setStrokeJoin(SkPaint::kBevel_Join); break;
    }
    if (stroke.dash) {
        auto& d = *stroke.dash;
        p.setPathEffect(SkDashPathEffect::Make(SkSpan<const float>(d.first.data(), d.first.size()), d.second));
    }
    canvas.drawPath(path, p);
}

void draw_pixmap(Pixmap& dst, int x, int y, const Pixmap& src, const PixmapPaint& pp, const SkMatrix& ts, const Mask* mask) {
    SkCanvas canvas = dst.canvas();
    if (mask) mask->apply(canvas);
    canvas.concat(ts);
    SkPaint p;
    p.setAlphaf(pp.opacity);
    p.setBlendMode(pp.blend_mode);
    canvas.drawImage(src.image(), (float)x, (float)y, sampling(pp.quality), &p);
}

// mark: driver -------------------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: oracle OUT_DIR [--list] [scene ...]\n"); return 2; }
    g_out_dir = argv[1];
    if (argc > 2 && strcmp(argv[2], "--list") == 0) {
        for (auto& s : scenes()) printf("%s\n", s.name);
        return 0;
    }
    int ran = 0;
    for (auto& s : scenes()) {
        bool wanted = argc == 2;
        for (int i = 2; i < argc; i++) if (strcmp(argv[i], s.name) == 0) wanted = true;
        if (!wanted) continue;
        s.run();
        ran++;
    }
    fprintf(stderr, "%d scenes\n", ran);
    return 0;
}
