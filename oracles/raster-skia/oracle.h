// oracle.h - render luce-browser-render raster's test scenes with Skia m144 (the exact
// libskia Ladybird 47c82b38d0 links), to produce the reference images the raster module must
// reproduce pixel for pixel.
//
// The helpers mirror tiny-skia's API (Pixmap, Paint, Stroke, Mask, PathBuilder, ...) so each
// scene is a line-by-line translation of the tiny-skia / raster test. Surfaces are what
// Ladybird paints into: kBGRA_8888 (kN32 on macOS arm64), premultiplied, sRGB color space.
// Output: PNGs, demultiplied like tiny-skia's Pixmap::encode_png (lossless round trip).
#pragma once

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkShader.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradientShader.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// mark: pixmaps ------------------------------------------------------------------------------

struct Pixmap {
    SkBitmap bitmap;
    int width() const { return bitmap.width(); }
    int height() const { return bitmap.height(); }
    SkCanvas canvas() { return SkCanvas(bitmap); }
    // premultiplied RGBA of pixel (x, y)
    void rgba(int x, int y, uint8_t out[4]) const {
        const uint8_t* p = (const uint8_t*)bitmap.getAddr32(x, y);
        out[0] = p[2]; out[1] = p[1]; out[2] = p[0]; out[3] = p[3];
    }
    sk_sp<SkImage> image() const { return bitmap.asImage(); }
};

SkImageInfo ladybird_info(int w, int h);
Pixmap new_pixmap(int w, int h);
// Fill with a color, as tiny-skia's Pixmap::fill (a raw premultiplied store, no blending).
void pixmap_fill(Pixmap& pm, SkColor4f color);
// Premultiplied RGBA pixels (tiny-skia's layout) from/to a pixmap.
std::vector<uint8_t> pixmap_rgba(const Pixmap& pm);
Pixmap pixmap_from_rgba(int w, int h, const std::vector<uint8_t>& rgba);

// Write `pm` to <out>/<name> (demultiplied RGBA PNG) and, beside it, <name>.rgba (raw
// premultiplied RGBA, width and height as two little-endian u32 first).
void save(const Pixmap& pm, const std::string& name);
// A grayscale mask image (tiny-skia's Mask::save_png writes a gray PNG).
void save_mask(const std::vector<uint8_t>& mask, int w, int h, const std::string& name);
extern std::string g_out_dir;

// mark: paths --------------------------------------------------------------------------------

// tiny-skia's PathBuilder, keeping conics as Skia paths do (the raster module's paths keep
// them too): conic_to, push_oval and push_circle emit real conic verbs.
struct PathBuilder {
    SkPathBuilder b;
    bool has_move = false;      // a contour is open (tiny-skia's move_to_required == false)
    bool last_is_move = false;
    SkPoint last_move{0, 0};
    SkPoint last{0, 0};
    int verbs = 0;
    void move_to(float x, float y);
    void line_to(float x, float y);
    void quad_to(float x1, float y1, float x, float y);
    void cubic_to(float x1, float y1, float x2, float y2, float x, float y);
    void conic_to(float x1, float y1, float x, float y, float w);
    void close();
    void push_rect(SkRect r);
    void push_oval(SkRect r);
    void push_circle(float x, float y, float r);
    std::optional<SkPath> finish();
    void inject_move();
};
SkPath path_from_rect(SkRect r);
std::optional<SkPath> path_from_circle(float cx, float cy, float r);
std::optional<SkPath> path_from_oval(SkRect r);

inline SkRect rect_xywh(float x, float y, float w, float h) { return SkRect::MakeXYWH(x, y, w, h); }
inline SkRect rect_ltrb(float l, float t, float r, float b) { return SkRect::MakeLTRB(l, t, r, b); }

// tiny-skia's Transform::from_row(sx, ky, kx, sy, tx, ty).
inline SkMatrix from_row(float sx, float ky, float kx, float sy, float tx, float ty) {
    return SkMatrix::MakeAll(sx, kx, tx, ky, sy, ty, 0, 0, 1);
}
inline SkMatrix from_translate(float tx, float ty) { return SkMatrix::Translate(tx, ty); }
inline SkMatrix from_scale(float sx, float sy) { return SkMatrix::Scale(sx, sy); }
inline SkMatrix identity() { return SkMatrix::I(); }

// mark: paints -------------------------------------------------------------------------------

enum class FillRule { Winding, EvenOdd };
enum class SpreadMode { Pad, Reflect, Repeat };
enum class FilterQuality { Nearest, Bilinear, Bicubic };

inline SkColor4f rgba8(int r, int g, int b, int a) {
    return SkColor4f::FromColor(SkColorSetARGB(a, r, g, b));
}

struct GradientStop { float pos; SkColor4f color; };

struct Paint {
    SkColor4f color = SkColors::kBlack;
    sk_sp<SkShader> shader;   // when set, replaces the color
    float shader_opacity = 1.0f; // a Pattern's opacity (the paint's alpha in Skia)
    SkBlendMode blend_mode = SkBlendMode::kSrcOver;
    bool anti_alias = true;
    bool dither = false;
    void set_color_rgba8(int r, int g, int b, int a) { color = rgba8(r, g, b, a); shader = nullptr; }
    void set_color(SkColor4f c) { color = c; shader = nullptr; }
    SkPaint to_sk() const;
};

sk_sp<SkShader> linear_gradient(SkPoint start, SkPoint end, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts);
sk_sp<SkShader> radial_gradient(SkPoint start, float start_radius, SkPoint end, float end_radius, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts);
sk_sp<SkShader> sweep_gradient(SkPoint center, float start_angle, float end_angle, const std::vector<GradientStop>& stops, SpreadMode mode, const SkMatrix& ts);
SkSamplingOptions sampling(FilterQuality q);
// A Pattern: the image shader. Its opacity goes to Paint::shader_opacity.
sk_sp<SkShader> pattern(const Pixmap& pm, SpreadMode mode, FilterQuality q, const SkMatrix& ts);

enum class LineCap { Butt, Round, Square };
enum class LineJoin { Miter, MiterClip, Round, Bevel };

struct Stroke {
    float width = 1.0f;
    float miter_limit = 4.0f;
    LineCap line_cap = LineCap::Butt;
    LineJoin line_join = LineJoin::Miter;
    std::optional<std::pair<std::vector<float>, float>> dash; // intervals, offset
};

// tiny-skia's StrokeDash::new(intervals, offset): None for an odd/short array, a negative
// interval, a non-positive sum or a non-finite offset.
std::optional<std::pair<std::vector<float>, float>> stroke_dash(std::vector<float> intervals, float offset);

// tiny-skia's Shader::apply_opacity on a gradient: every stop's alpha *= opacity (0..1).
void apply_opacity(std::vector<GradientStop>& stops, float opacity);

// mark: masks (clips) ------------------------------------------------------------------------

// A tiny-skia Mask built from paths is a Skia clip: each fill_path / intersect_path is a
// clipPath (in device space) with the given anti-aliasing.
struct Mask {
    int w, h;
    struct Op { SkPath path; bool aa; };
    std::vector<Op> ops;
    void fill_path(const SkPath& path, FillRule rule, bool aa, const SkMatrix& ts);
    void intersect_path(const SkPath& path, FillRule rule, bool aa, const SkMatrix& ts);
    void apply(SkCanvas& canvas) const;
    // The mask's coverage (one byte per pixel), rasterized by Skia: the clip ops applied to
    // an A8 surface, filled opaque.
    std::vector<uint8_t> coverage() const;
};
inline Mask new_mask(int w, int h) { return Mask{w, h, {}}; }

// Not Skia operations: tiny-skia's pixel-level mask utilities, computed in C++ with
// tiny-skia's exact formulas from Skia-rendered pixels.
enum class MaskType { Alpha, Luminance };
// Mask::from_pixmap (src/mask.rs).
std::vector<uint8_t> mask_from_pixmap(const Pixmap& pm, MaskType type);
// Pixmap::apply_mask: lowp load_mask_u8 + load_dst + destination_in + store,
// i.e. every premultiplied channel d = div255(d * m), div255(v) = (v + 255) >> 8.
void apply_mask(Pixmap& pm, const std::vector<uint8_t>& mask);

// PixmapRef::clone_rect: the part of the pixmap inside (x, y, w, h), clipped to its bounds;
// nullopt when they do not intersect.
std::optional<Pixmap> clone_rect(const Pixmap& pm, int x, int y, int w, int h);

// Check a pixmap against tiny-skia-style expected pixels (ColorU8::from_rgba(..).premultiply()
// given as demultiplied RGBA quads); prints the mismatches to stderr.
void check_pixels(const Pixmap& pm, const std::vector<std::array<int, 4>>& expected, const char* what);

// mark: drawing ------------------------------------------------------------------------------

void fill_path(Pixmap& pm, const SkPath& path, const Paint& paint, FillRule rule, const SkMatrix& ts, const Mask* mask);
void fill_rect(Pixmap& pm, SkRect rect, const Paint& paint, const SkMatrix& ts, const Mask* mask);
void stroke_path(Pixmap& pm, const SkPath& path, const Paint& paint, const Stroke& stroke, const SkMatrix& ts, const Mask* mask);

struct PixmapPaint {
    float opacity = 1.0f;
    SkBlendMode blend_mode = SkBlendMode::kSrcOver;
    FilterQuality quality = FilterQuality::Nearest;
};
void draw_pixmap(Pixmap& dst, int x, int y, const Pixmap& src, const PixmapPaint& paint, const SkMatrix& ts, const Mask* mask);

// mark: scenes -------------------------------------------------------------------------------

struct Scene { const char* name; std::function<void()> run; };
std::vector<Scene>& scenes();
struct SceneRegistrar { SceneRegistrar(const char* name, std::function<void()> run) { scenes().push_back({name, std::move(run)}); } };
#define SCENE(id) static void scene_##id(); static SceneRegistrar reg_##id(#id, scene_##id); static void scene_##id()
