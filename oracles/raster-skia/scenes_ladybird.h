// scenes_ladybird.h - helpers of the Ladybird-style scenes (scenes_ladybird_*.cpp), the
// C++ twins of the helpers in luce-browser-render's tests/raster/ladybird.lucb.
//
// Every value is computed with the same float operations, in the same order, as the Luce
// test; the including file turns off floating-point contraction (Luce never fuses a
// multiply and an add). Rounded rectangles, circles and ellipses are real conics on both
// sides (PathBuilder.conic_to / push_oval / push_circle).
#pragma once

#include "oracle.h"

// The conic weight of a quarter ellipse, sqrt(2) / 2.
static const float quarter_weight = 0.707106781f;

// A pixmap of the scene's size; `background` fills it with an opaque off-white first.
static inline Pixmap lb_pixmap(int w, int h, bool background) {
    Pixmap pm = new_pixmap(w, h);
    if (background) pixmap_fill(pm, rgba8(240, 236, 226, 255));
    return pm;
}

static inline Paint solid(int r, int g, int b, int a, bool aa) {
    Paint paint;
    paint.set_color_rgba8(r, g, b, a);
    paint.anti_alias = aa;
    return paint;
}

// A rounded rectangle: `radii` holds the (x, y) radii of the top-left, top-right,
// bottom-right and bottom-left corners; a corner with both radii zero is square.
static inline SkPath rounded_rect_radii(float x, float y, float w, float h, std::array<float, 8> radii) {
    float right = x + w;
    float bottom = y + h;
    PathBuilder pb;
    pb.move_to(x + radii[0], y);
    pb.line_to(right - radii[2], y);
    if (radii[2] > 0.0f || radii[3] > 0.0f) pb.conic_to(right, y, right, y + radii[3], quarter_weight);
    pb.line_to(right, bottom - radii[5]);
    if (radii[4] > 0.0f || radii[5] > 0.0f) pb.conic_to(right, bottom, right - radii[4], bottom, quarter_weight);
    pb.line_to(x + radii[6], bottom);
    if (radii[6] > 0.0f || radii[7] > 0.0f) pb.conic_to(x, bottom, x, bottom - radii[7], quarter_weight);
    pb.line_to(x, y + radii[1]);
    if (radii[0] > 0.0f || radii[1] > 0.0f) pb.conic_to(x, y, x + radii[0], y, quarter_weight);
    pb.close();
    return *pb.finish();
}

static inline SkPath elliptical_rect(float x, float y, float w, float h, float rx, float ry) {
    return rounded_rect_radii(x, y, w, h, {rx, ry, rx, ry, rx, ry, rx, ry});
}

static inline SkPath rounded_rect(float x, float y, float w, float h, float r) { return elliptical_rect(x, y, w, h, r, r); }

// push_circle / push_oval: four conics of weight sqrt(2) / 2.
static inline SkPath circle(float cx, float cy, float r) { return *path_from_circle(cx, cy, r); }
static inline SkPath oval(float x, float y, float w, float h) { return *path_from_oval(rect_xywh(x, y, w, h)); }
static inline SkPath rect_path(float x, float y, float w, float h) { return path_from_rect(rect_xywh(x, y, w, h)); }

static inline void fill(Pixmap& pm, const SkPath& path, const Paint& paint) {
    fill_path(pm, path, paint, FillRule::Winding, identity(), nullptr);
}

static inline void fill_ts(Pixmap& pm, const SkPath& path, const Paint& paint, const SkMatrix& ts) {
    fill_path(pm, path, paint, FillRule::Winding, ts, nullptr);
}

static inline void stroke(Pixmap& pm, const SkPath& path, const Paint& paint, const Stroke& style) {
    stroke_path(pm, path, paint, style, identity(), nullptr);
}

// A test image of `size` x `size` premultiplied pixels: a red/green ramp with a blue
// checker, a translucent bottom-right quadrant and a transparent 2x2 top-left corner.
static inline Pixmap source_image(int size) {
    size_t n = (size_t)size;
    std::vector<uint8_t> data(n * n * 4);
    for (size_t y = 0; y < n; y++)
        for (size_t x = 0; x < n; x++) {
            size_t r = x * 255 / (n - 1), g = y * 255 / (n - 1), b = ((x + y) % 4) * 60, a = 255;
            if (x >= n / 2 && y >= n / 2) {
                a = 160;
                r = r * a / 255;
                g = g * a / 255;
                b = b * a / 255;
            }
            if (x < 2 && y < 2) r = g = b = a = 0;
            size_t i = (y * n + x) * 4;
            data[i + 0] = (uint8_t)r;
            data[i + 1] = (uint8_t)g;
            data[i + 2] = (uint8_t)b;
            data[i + 3] = (uint8_t)a;
        }
    return pixmap_from_rgba(size, size, data);
}

// Six lines, offset by `o`: horizontal, vertical, 45 degrees, 30 degrees, nearly horizontal
// and nearly vertical, as separate contours of one path.
static inline SkPath line_set(float o) {
    PathBuilder pb;
    pb.move_to(10.0f + o, 10.0f + o);
    pb.line_to(90.0f + o, 10.0f + o);
    pb.move_to(10.0f + o, 20.0f + o);
    pb.line_to(10.0f + o, 90.0f + o);
    pb.move_to(20.0f + o, 20.0f + o);
    pb.line_to(60.0f + o, 60.0f + o);
    pb.move_to(20.0f + o, 70.0f + o);
    pb.line_to(71.96f + o, 40.0f + o);
    pb.move_to(20.0f + o, 85.0f + o);
    pb.line_to(90.0f + o, 88.0f + o);
    pb.move_to(80.0f + o, 15.0f + o);
    pb.line_to(83.0f + o, 75.0f + o);
    return *pb.finish();
}

// An open zigzag polyline for the join tests.
static inline SkPath zigzag() {
    PathBuilder pb;
    pb.move_to(10.0, 50.0);
    pb.line_to(30.0, 15.0);
    pb.line_to(50.0, 50.0);
    pb.line_to(70.0, 20.0);
    pb.line_to(80.0, 55.0);
    pb.line_to(110.0, 30.0);
    return *pb.finish();
}

static inline SkPath triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    PathBuilder pb;
    pb.move_to(x0, y0);
    pb.line_to(x1, y1);
    pb.line_to(x2, y2);
    pb.close();
    return *pb.finish();
}

// A glyph like an 'o' in a 16 x 20 box: two elliptical cubic contours, the inner one a hole
// under the even-odd rule.
static inline void push_glyph_o(PathBuilder& pb) {
    pb.move_to(16.0, 10.0);
    pb.cubic_to(16.0, 15.52, 12.42, 20.0, 8.0, 20.0);
    pb.cubic_to(3.58, 20.0, 0.0, 15.52, 0.0, 10.0);
    pb.cubic_to(0.0, 4.48, 3.58, 0.0, 8.0, 0.0);
    pb.cubic_to(12.42, 0.0, 16.0, 4.48, 16.0, 10.0);
    pb.close();
    pb.move_to(12.5, 10.0);
    pb.cubic_to(12.5, 13.59, 10.49, 16.5, 8.0, 16.5);
    pb.cubic_to(5.51, 16.5, 3.5, 13.59, 3.5, 10.0);
    pb.cubic_to(3.5, 6.41, 5.51, 3.5, 8.0, 3.5);
    pb.cubic_to(10.49, 3.5, 12.5, 6.41, 12.5, 10.0);
    pb.close();
}

// A glyph like an 'e' in a 16 x 20 box: a quadratic outline with a bar and an eye.
static inline void push_glyph_e(PathBuilder& pb) {
    pb.move_to(15.0, 11.0);
    pb.line_to(3.8, 11.0);
    pb.quad_to(4.2, 16.8, 9.0, 16.8);
    pb.quad_to(12.0, 16.8, 13.6, 14.6);
    pb.line_to(15.4, 15.9);
    pb.quad_to(13.0, 20.0, 8.8, 20.0);
    pb.quad_to(0.2, 20.0, 0.2, 10.0);
    pb.quad_to(0.2, 0.0, 8.0, 0.0);
    pb.quad_to(15.0, 0.0, 15.0, 9.0);
    pb.close();
    pb.move_to(3.9, 8.0);
    pb.quad_to(4.3, 3.2, 8.0, 3.2);
    pb.quad_to(11.3, 3.2, 11.5, 8.0);
    pb.close();
}

// Draw a glyph scaled by `s` (20 * s pixels tall) at (x, y), even-odd.
static inline void draw_glyph(Pixmap& pm, bool e, float s, float x, float y, const Paint& paint) {
    PathBuilder pb;
    if (e) push_glyph_e(pb);
    else push_glyph_o(pb);
    fill_path(pm, *pb.finish(), paint, FillRule::EvenOdd, from_row(s, 0.0, 0.0, s, x, y), nullptr);
}
