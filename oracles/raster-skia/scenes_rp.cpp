// scenes_rp.cpp - raster pipeline scenes (SkRasterPipelineBlitter, lowp/highp stages, blend
// modes, gradients, dither, image shaders) rendered with Skia m144. The Luce twin of every scene
// is in tests/raster/skia_pipeline.lucb; the PNGs go to <out>/pipeline/.
//
// Shapes are non-AA pixel-aligned rects (the pipeline is what is tested), except the *_aa scenes,
// whose fractional rects exercise blitAntiH / blitV coverage (scale_1_float / lerp_1_float /
// scale_u8 / lerp_u8).
#include "oracle.h"

// mark: helpers ------------------------------------------------------------------------------

// A color from bytes, as the raster module's Color::from_rgba8 makes it: SkColor4f::FromColor
// (each byte times 1/255.0f).
static SkColor4f rp_rgba8(int r, int g, int b, int a) { return rgba8(r, g, b, a); }

// A deterministic premultiplied background, written directly into the pixels (no drawing).
static void rp_background(Pixmap& pm) {
    for (int y = 0; y < pm.height(); y++)
        for (int x = 0; x < pm.width(); x++) {
            static const int alphas[5] = {255, 0, 128, 200, 37};
            int a = alphas[((x / 3) + (y / 5)) % 5];
            int r = (x * 7 + y * 3) & 255, g = (x * 2 + y * 5 + 40) & 255, b = (x * x + y * 11) & 255;
            uint8_t* p = (uint8_t*)pm.bitmap.getAddr32(x, y);
            p[0] = (uint8_t)(b * a / 255); p[1] = (uint8_t)(g * a / 255); p[2] = (uint8_t)(r * a / 255); p[3] = (uint8_t)a;
        }
}

// A deterministic premultiplied 24x20 image with translucent pixels.
static Pixmap rp_image() {
    Pixmap pm = new_pixmap(24, 20);
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 24; x++) {
            int a = ((x + y) % 7 == 0) ? 90 : ((x * 3 + y) % 11 == 0 ? 0 : 255);
            int r = (x * 11) & 255, g = (y * 13) & 255, b = ((x ^ y) * 9) & 255;
            uint8_t* p = (uint8_t*)pm.bitmap.getAddr32(x, y);
            p[0] = (uint8_t)(b * a / 255); p[1] = (uint8_t)(g * a / 255); p[2] = (uint8_t)(r * a / 255); p[3] = (uint8_t)a;
        }
    return pm;
}

// Skia's test hook gForceHighPrecisionRasterPipeline is a local symbol of libskia: find it from
// an exported function's address and the offsets `nm` prints for both (0x3f52a8 and 0x213f8).
static bool* rp_force_highp_flag() {
    using MakeLinearFn = sk_sp<SkShader> (*)(const SkPoint[2], const SkColor4f[], sk_sp<SkColorSpace>, const SkScalar[], int, SkTileMode,
                                             const SkGradientShader::Interpolation&, const SkMatrix*);
    MakeLinearFn f = &SkGradientShader::MakeLinear;
    return (bool*)((uintptr_t)f - 0x213f8 + 0x3f52a8);
}
// Runs `draw` with every raster pipeline built in high precision.
static void rp_highp(const std::function<void()>& draw) {
    *rp_force_highp_flag() = true;
    draw();
    *rp_force_highp_flag() = false;
}

static const SkBlendMode kModes[29] = {
    SkBlendMode::kClear, SkBlendMode::kSrc, SkBlendMode::kDst, SkBlendMode::kSrcOver, SkBlendMode::kDstOver,
    SkBlendMode::kSrcIn, SkBlendMode::kDstIn, SkBlendMode::kSrcOut, SkBlendMode::kDstOut, SkBlendMode::kSrcATop,
    SkBlendMode::kDstATop, SkBlendMode::kXor, SkBlendMode::kPlus, SkBlendMode::kModulate, SkBlendMode::kScreen,
    SkBlendMode::kOverlay, SkBlendMode::kDarken, SkBlendMode::kLighten, SkBlendMode::kColorDodge, SkBlendMode::kColorBurn,
    SkBlendMode::kHardLight, SkBlendMode::kSoftLight, SkBlendMode::kDifference, SkBlendMode::kExclusion, SkBlendMode::kMultiply,
    SkBlendMode::kHue, SkBlendMode::kSaturation, SkBlendMode::kColor, SkBlendMode::kLuminosity,
};

// Every blend mode in a 6x5 grid of 22x22 cells: `paint` (its blend mode replaced) fills an 18x18
// rect in each (or a fractional one with anti-aliasing when `aa`).
static void rp_blend_grid(Paint paint, bool aa, const char* image) {
    Pixmap pm = new_pixmap(132, 110);
    rp_background(pm);
    for (int i = 0; i < 29; i++) {
        float x = (float)(i % 6) * 22.0f, y = (float)(i / 6) * 22.0f;
        paint.blend_mode = kModes[i];
        paint.anti_alias = aa;
        SkRect r = aa ? rect_xywh(x + 2.3f, y + 2.6f, 16.4f, 15.7f) : rect_xywh(x + 2.0f, y + 2.0f, 18.0f, 18.0f);
        fill_rect(pm, r, paint, identity(), nullptr);
    }
    save(pm, image);
}

static std::vector<GradientStop> two_stops() { return {{0.0f, rp_rgba8(50, 127, 150, 200)}, {1.0f, rp_rgba8(220, 140, 75, 180)}}; }
static std::vector<GradientStop> three_stops() {
    return {{0.0f, rp_rgba8(50, 127, 150, 200)}, {0.5f, rp_rgba8(220, 140, 75, 180)}, {1.0f, rp_rgba8(40, 180, 55, 160)}};
}
static std::vector<GradientStop> uneven_stops() {
    return {{0.1f, rp_rgba8(50, 127, 150, 200)}, {0.3f, rp_rgba8(220, 140, 75, 180)}, {0.3f, rp_rgba8(10, 20, 250, 255)}, {0.85f, rp_rgba8(40, 180, 55, 160)}};
}
static std::vector<GradientStop> opaque_stops() {
    return {{0.0f, rp_rgba8(250, 10, 10, 255)}, {0.3f, rp_rgba8(10, 250, 10, 255)}, {0.6f, rp_rgba8(10, 10, 250, 255)}, {1.0f, rp_rgba8(200, 200, 30, 255)}};
}
static std::vector<GradientStop> many_stops() {
    std::vector<GradientStop> s;
    for (int i = 0; i < 11; i++) {
        float t = (float)(i * i) / 100.0f;
        s.push_back({t, rp_rgba8((i * 50) & 255, (i * 90 + 30) & 255, (255 - i * 20) & 255, 120 + i * 13)});
    }
    return s;
}

// A gradient fill of a whole 120x100 pixmap (a non-AA rect) over the background.
static void rp_gradient(sk_sp<SkShader> shader, bool dither, const char* image, float opacity = 1.0f) {
    Pixmap pm = new_pixmap(120, 100);
    rp_background(pm);
    Paint paint;
    paint.anti_alias = false;
    paint.dither = dither;
    paint.shader = shader;
    paint.shader_opacity = opacity;
    fill_rect(pm, rect_xywh(0, 0, 120, 100), paint, identity(), nullptr);
    save(pm, image);
}

static sk_sp<SkShader> linear_premul(SkPoint a, SkPoint b, const std::vector<GradientStop>& stops, SpreadMode mode) {
    std::vector<SkColor4f> colors; std::vector<float> pos;
    for (auto& s : stops) { colors.push_back(s.color); pos.push_back(s.pos); }
    SkPoint pts[2] = {a, b};
    SkGradientShader::Interpolation interp;
    interp.fInPremul = SkGradientShader::Interpolation::InPremul::kYes;
    SkTileMode tm = mode == SpreadMode::Pad ? SkTileMode::kClamp : mode == SpreadMode::Repeat ? SkTileMode::kRepeat : SkTileMode::kMirror;
    return SkGradientShader::MakeLinear(pts, colors.data(), nullptr, pos.data(), (int)colors.size(), tm, interp, nullptr);
}

// mark: blend modes --------------------------------------------------------------------------

SCENE(rp_blend_solid) {
    Paint paint; paint.set_color(rp_rgba8(50, 127, 150, 200));
    rp_blend_grid(paint, false, "pipeline/blend-solid.png");
}
SCENE(rp_blend_solid_opaque) {
    Paint paint; paint.set_color(rp_rgba8(220, 140, 75, 255));
    rp_blend_grid(paint, false, "pipeline/blend-solid-opaque.png");
}
SCENE(rp_blend_solid_aa) {
    Paint paint; paint.set_color(rp_rgba8(50, 127, 150, 200));
    rp_blend_grid(paint, true, "pipeline/blend-solid-aa.png");
}
SCENE(rp_blend_gradient) {
    Paint paint;
    paint.shader = linear_gradient({0, 0}, {132, 110}, three_stops(), SpreadMode::Repeat, identity());
    rp_blend_grid(paint, false, "pipeline/blend-gradient.png");
}
SCENE(rp_blend_gradient_opaque) {
    Paint paint;
    paint.shader = linear_gradient({0, 0}, {40, 20}, opaque_stops(), SpreadMode::Reflect, identity());
    rp_blend_grid(paint, false, "pipeline/blend-gradient-opaque.png");
}
SCENE(rp_blend_gradient_aa) {
    Paint paint;
    paint.shader = linear_gradient({0, 0}, {132, 110}, three_stops(), SpreadMode::Repeat, identity());
    rp_blend_grid(paint, true, "pipeline/blend-gradient-aa.png");
}
SCENE(rp_blend_gradient_dither) {
    Paint paint;
    paint.dither = true;
    paint.shader = linear_gradient({0, 0}, {132, 110}, three_stops(), SpreadMode::Repeat, identity());
    rp_blend_grid(paint, false, "pipeline/blend-gradient-dither.png");
}

// mark: linear gradients ---------------------------------------------------------------------

SCENE(rp_linear_two_pad) { rp_gradient(linear_gradient({10, 20}, {100, 70}, two_stops(), SpreadMode::Pad, identity()), false, "pipeline/linear-two-pad.png"); }
SCENE(rp_linear_two_pad_dither) { rp_gradient(linear_gradient({10, 20}, {100, 70}, two_stops(), SpreadMode::Pad, identity()), true, "pipeline/linear-two-pad-dither.png"); }
SCENE(rp_linear_two_repeat) { rp_gradient(linear_gradient({10, 20}, {40, 30}, two_stops(), SpreadMode::Repeat, identity()), false, "pipeline/linear-two-repeat.png"); }
SCENE(rp_linear_two_reflect) { rp_gradient(linear_gradient({10, 20}, {40, 30}, two_stops(), SpreadMode::Reflect, identity()), false, "pipeline/linear-two-reflect.png"); }
SCENE(rp_linear_three_pad) { rp_gradient(linear_gradient({0, 0}, {120, 100}, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/linear-three-pad.png"); }
SCENE(rp_linear_three_reflect_dither) { rp_gradient(linear_gradient({30, 0}, {70, 30}, three_stops(), SpreadMode::Reflect, identity()), true, "pipeline/linear-three-reflect-dither.png"); }
SCENE(rp_linear_uneven_pad) { rp_gradient(linear_gradient({0, 50}, {120, 50}, uneven_stops(), SpreadMode::Pad, identity()), false, "pipeline/linear-uneven-pad.png"); }
SCENE(rp_linear_uneven_repeat) { rp_gradient(linear_gradient({5, 5}, {45, 25}, uneven_stops(), SpreadMode::Repeat, identity()), false, "pipeline/linear-uneven-repeat.png"); }
SCENE(rp_linear_uneven_repeat_dither) { rp_gradient(linear_gradient({5, 5}, {45, 25}, uneven_stops(), SpreadMode::Repeat, identity()), true, "pipeline/linear-uneven-repeat-dither.png"); }
SCENE(rp_linear_many) { rp_gradient(linear_gradient({0, 0}, {120, 30}, many_stops(), SpreadMode::Reflect, identity()), false, "pipeline/linear-many.png"); }
SCENE(rp_linear_opaque_rotated) { rp_gradient(linear_gradient({0, 0}, {60, 0}, opaque_stops(), SpreadMode::Repeat, from_row(0.8f, 0.5f, -0.4f, 1.2f, 30.0f, 10.0f)), false, "pipeline/linear-opaque-rotated.png"); }
SCENE(rp_linear_scaled) { rp_gradient(linear_gradient({0, 0}, {10, 0}, three_stops(), SpreadMode::Reflect, from_row(3.0f, 0.0f, 0.0f, 2.0f, 7.5f, 0.0f)), false, "pipeline/linear-scaled.png"); }
SCENE(rp_linear_premul) { rp_gradient(linear_premul({10, 10}, {110, 90}, uneven_stops(), SpreadMode::Pad), false, "pipeline/linear-premul.png"); }
SCENE(rp_linear_premul_two) { rp_gradient(linear_premul({10, 10}, {110, 90}, two_stops(), SpreadMode::Repeat), true, "pipeline/linear-premul-two.png"); }

// mark: radial and two-point conical gradients -----------------------------------------------

SCENE(rp_radial_pad) { rp_gradient(radial_gradient({60, 50}, 0, {60, 50}, 45, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/radial-pad.png"); }
SCENE(rp_radial_repeat_dither) { rp_gradient(radial_gradient({60, 50}, 0, {60, 50}, 17, uneven_stops(), SpreadMode::Repeat, identity()), true, "pipeline/radial-repeat-dither.png"); }
SCENE(rp_radial_scaled) { rp_gradient(radial_gradient({20, 20}, 0, {20, 20}, 15, two_stops(), SpreadMode::Reflect, from_row(1.5f, 0.2f, 0.0f, 1.0f, 10.0f, 5.0f)), false, "pipeline/radial-scaled.png"); }
SCENE(rp_conical_concentric) { rp_gradient(radial_gradient({60, 50}, 10, {60, 50}, 40, three_stops(), SpreadMode::Repeat, identity()), false, "pipeline/conical-concentric.png"); }
SCENE(rp_conical_well_behaved) { rp_gradient(radial_gradient({50, 50}, 0, {60, 50}, 45, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/conical-well-behaved.png"); }
SCENE(rp_conical_greater) { rp_gradient(radial_gradient({30, 40}, 5, {80, 60}, 20, three_stops(), SpreadMode::Reflect, identity()), false, "pipeline/conical-greater.png"); }
SCENE(rp_conical_smaller) { rp_gradient(radial_gradient({30, 40}, 30, {80, 60}, 5, two_stops(), SpreadMode::Pad, identity()), true, "pipeline/conical-smaller.png"); }
SCENE(rp_conical_strip) { rp_gradient(radial_gradient({30, 50}, 20, {90, 45}, 20, uneven_stops(), SpreadMode::Repeat, identity()), false, "pipeline/conical-strip.png"); }
SCENE(rp_conical_focal_on_circle) { rp_gradient(radial_gradient({40, 50}, 0, {70, 50}, 30, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/conical-focal-on-circle.png"); }

// mark: sweep gradients ----------------------------------------------------------------------

SCENE(rp_sweep_full) { rp_gradient(sweep_gradient({60, 50}, 0, 360, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/sweep-full.png"); }
SCENE(rp_sweep_partial) { rp_gradient(sweep_gradient({60, 50}, 45, 270, uneven_stops(), SpreadMode::Reflect, identity()), false, "pipeline/sweep-partial.png"); }
SCENE(rp_sweep_start_zero) { rp_gradient(sweep_gradient({60, 50}, 0, 180, two_stops(), SpreadMode::Repeat, identity()), false, "pipeline/sweep-start-zero.png"); }
SCENE(rp_sweep_dither) { rp_gradient(sweep_gradient({50, 40}, 0, 360, many_stops(), SpreadMode::Pad, from_row(1.0f, 0.3f, -0.3f, 1.0f, 0.0f, 0.0f)), true, "pipeline/sweep-dither.png"); }

// mark: patterns (image shaders through the pipeline) ----------------------------------------

// The image as a pattern filling a 120x100 pixmap over the background.
static void rp_pattern(SpreadMode mode, FilterQuality q, SkBlendMode blend, float opacity, const SkMatrix& ts, const char* image) {
    Pixmap img = rp_image();
    Pixmap pm = new_pixmap(120, 100);
    rp_background(pm);
    Paint paint;
    paint.anti_alias = false;
    paint.shader = pattern(img, mode, q, ts);
    paint.shader_opacity = opacity;
    paint.blend_mode = blend;
    fill_rect(pm, rect_xywh(0, 0, 120, 100), paint, identity(), nullptr);
    save(pm, image);
}

static SkMatrix rp_rot() { return from_row(2.2f, 0.9f, -0.7f, 1.9f, 20.0f, 3.0f); }

SCENE(rp_pattern_bicubic_pad) { rp_pattern(SpreadMode::Pad, FilterQuality::Bicubic, SkBlendMode::kSrcOver, 1.0f, rp_rot(), "pipeline/pattern-bicubic-pad.png"); }
SCENE(rp_pattern_bicubic_repeat) { rp_pattern(SpreadMode::Repeat, FilterQuality::Bicubic, SkBlendMode::kSrcOver, 1.0f, rp_rot(), "pipeline/pattern-bicubic-repeat.png"); }
SCENE(rp_pattern_bicubic_reflect) { rp_pattern(SpreadMode::Reflect, FilterQuality::Bicubic, SkBlendMode::kSrcOver, 1.0f, rp_rot(), "pipeline/pattern-bicubic-reflect.png"); }
SCENE(rp_pattern_bicubic_opacity) { rp_pattern(SpreadMode::Repeat, FilterQuality::Bicubic, SkBlendMode::kSrcOver, 0.6f, from_row(3.0f, 0.0f, 0.0f, 2.5f, 1.0f, 2.0f), "pipeline/pattern-bicubic-opacity.png"); }
SCENE(rp_pattern_bilinear_pad_src) { rp_pattern(SpreadMode::Pad, FilterQuality::Bilinear, SkBlendMode::kSrc, 1.0f, rp_rot(), "pipeline/pattern-bilinear-pad-src.png"); }
SCENE(rp_pattern_bilinear_repeat_multiply) { rp_pattern(SpreadMode::Repeat, FilterQuality::Bilinear, SkBlendMode::kMultiply, 1.0f, rp_rot(), "pipeline/pattern-bilinear-repeat-multiply.png"); }
SCENE(rp_pattern_bilinear_reflect_screen) { rp_pattern(SpreadMode::Reflect, FilterQuality::Bilinear, SkBlendMode::kScreen, 0.7f, rp_rot(), "pipeline/pattern-bilinear-reflect-screen.png"); }
SCENE(rp_pattern_nearest_repeat_src) { rp_pattern(SpreadMode::Repeat, FilterQuality::Nearest, SkBlendMode::kSrc, 1.0f, rp_rot(), "pipeline/pattern-nearest-repeat-src.png"); }
SCENE(rp_pattern_nearest_reflect_xor) { rp_pattern(SpreadMode::Reflect, FilterQuality::Nearest, SkBlendMode::kXor, 1.0f, from_row(2.0f, 0.0f, 0.0f, 2.0f, 3.0f, 1.0f), "pipeline/pattern-nearest-reflect-xor.png"); }
SCENE(rp_pattern_nearest_pad_darken) { rp_pattern(SpreadMode::Pad, FilterQuality::Nearest, SkBlendMode::kDarken, 1.0f, from_row(-1.5f, 0.0f, 0.0f, 1.5f, 80.0f, 10.0f), "pipeline/pattern-nearest-pad-darken.png"); }

// mark: forced high precision (gForceHighPrecisionRasterPipeline) -----------------------------

SCENE(rp_highp_blend_solid) { rp_highp([] { Paint paint; paint.set_color(rp_rgba8(50, 127, 150, 200)); rp_blend_grid(paint, false, "pipeline/highp-blend-solid.png"); }); }
SCENE(rp_highp_blend_solid_aa) { rp_highp([] { Paint paint; paint.set_color(rp_rgba8(50, 127, 150, 200)); rp_blend_grid(paint, true, "pipeline/highp-blend-solid-aa.png"); }); }
SCENE(rp_highp_blend_gradient) {
    rp_highp([] {
        Paint paint;
        paint.shader = linear_gradient({0, 0}, {132, 110}, three_stops(), SpreadMode::Repeat, identity());
        rp_blend_grid(paint, false, "pipeline/highp-blend-gradient.png");
    });
}
SCENE(rp_highp_linear_uneven_pad) { rp_highp([] { rp_gradient(linear_gradient({0, 50}, {120, 50}, uneven_stops(), SpreadMode::Pad, identity()), false, "pipeline/highp-linear-uneven-pad.png"); }); }
SCENE(rp_highp_radial_pad) { rp_highp([] { rp_gradient(radial_gradient({60, 50}, 0, {60, 50}, 45, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/highp-radial-pad.png"); }); }
SCENE(rp_highp_sweep_full) { rp_highp([] { rp_gradient(sweep_gradient({60, 50}, 0, 360, three_stops(), SpreadMode::Pad, identity()), false, "pipeline/highp-sweep-full.png"); }); }
SCENE(rp_highp_pattern_bilinear_pad_src) { rp_highp([] { rp_pattern(SpreadMode::Pad, FilterQuality::Bilinear, SkBlendMode::kSrc, 1.0f, rp_rot(), "pipeline/highp-pattern-bilinear-pad-src.png"); }); }

// mark: tiny-skia's patterns without anti-aliasing ------------------------------------------

// tiny-skia's pattern tests with the triangle drawn without anti-aliasing (so the image is the
// same on both sides whatever the AA scan converter does).
static void rp_ts_pattern(SpreadMode mode, FilterQuality quality, const SkMatrix& ts, const char* image) {
    Paint tp;
    tp.set_color(rp_rgba8(50, 127, 150, 200));
    tp.anti_alias = false;
    PathBuilder pb;
    pb.move_to(0.0, 20.0);
    pb.line_to(20.0, 20.0);
    pb.line_to(10.0, 0.0);
    pb.close();
    Pixmap triangle = new_pixmap(20, 20);
    fill_path(triangle, *pb.finish(), tp, FillRule::Winding, identity(), nullptr);

    Paint paint;
    paint.anti_alias = false;
    paint.shader = pattern(triangle, mode, quality, ts);
    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0)), paint, FillRule::Winding, identity(), nullptr);
    save(pixmap, image);
}
SCENE(rp_ts_pattern_pad_nearest) { rp_ts_pattern(SpreadMode::Pad, FilterQuality::Nearest, identity(), "rp-ts/pattern-pad-nearest.png"); }
SCENE(rp_ts_pattern_repeat_nearest) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Nearest, identity(), "rp-ts/pattern-repeat-nearest.png"); }
SCENE(rp_ts_pattern_reflect_nearest) { rp_ts_pattern(SpreadMode::Reflect, FilterQuality::Nearest, identity(), "rp-ts/pattern-reflect-nearest.png"); }
SCENE(rp_ts_pattern_pad_bicubic) { rp_ts_pattern(SpreadMode::Pad, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "rp-ts/pattern-pad-bicubic.png"); }
SCENE(rp_ts_pattern_repeat_bicubic) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "rp-ts/pattern-repeat-bicubic.png"); }
SCENE(rp_ts_pattern_reflect_bicubic) { rp_ts_pattern(SpreadMode::Reflect, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "rp-ts/pattern-reflect-bicubic.png"); }
SCENE(rp_ts_pattern_filter_nearest) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Nearest, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "rp-ts/pattern-filter-nearest.png"); }
SCENE(rp_ts_pattern_filter_bilinear) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Bilinear, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "rp-ts/pattern-filter-bilinear.png"); }
SCENE(rp_ts_pattern_filter_bicubic) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Bicubic, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "rp-ts/pattern-filter-bicubic.png"); }
SCENE(rp_ts_pattern_bilinear_pad_scaled) { rp_ts_pattern(SpreadMode::Pad, FilterQuality::Bilinear, from_row(4.3, 0.0, 0.0, 3.7, 12.25, 7.5), "rp-ts/pattern-bilinear-pad-scaled.png"); }
SCENE(rp_ts_pattern_nearest_int_translate) { rp_ts_pattern(SpreadMode::Repeat, FilterQuality::Bilinear, from_row(1.0, 0.0, 0.0, 1.0, 3.0, 5.0), "rp-ts/pattern-nearest-int-translate.png"); }
SCENE(rp_ts_pattern_bicubic_identity) { rp_ts_pattern(SpreadMode::Reflect, FilterQuality::Bicubic, identity(), "rp-ts/pattern-bicubic-identity.png"); }

// mark: tests_core's blend test ---------------------------------------------------------------

// raster's tests_core "blend lowp" / "blend highp": a 1x1 pixmap filled with (50, 127, 150, 200),
// then (220, 140, 75, 180) drawn with each blend mode; prints the premultiplied results.
SCENE(rp_unit_blend) {
    for (int highp = 0; highp < 2; highp++) {
        for (int i = 0; i < 29; i++) {
            Pixmap pm = new_pixmap(1, 1);
            pixmap_fill(pm, rgba8(50, 127, 150, 200));
            Paint paint;
            paint.set_color(rgba8(220, 140, 75, 180));
            paint.blend_mode = kModes[i];
            paint.anti_alias = false;
            auto draw = [&] { fill_rect(pm, rect_xywh(0, 0, 1, 1), paint, identity(), nullptr); };
            if (highp) rp_highp(draw); else draw();
            uint8_t px[4];
            pm.rgba(0, 0, px);
            fprintf(stderr, "%s %d: %d, %d, %d, %d\n", highp ? "highp" : "lowp", i, px[0], px[1], px[2], px[3]);
        }
    }
}
