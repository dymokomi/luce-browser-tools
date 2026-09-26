// scenes_ladybird_shapes.cpp - Ladybird-style scenes: rounded rectangles, circles and
// ellipses, thin lines, strokes and dashes, translucent overlaps, glyph-like outlines,
// transformed paths and gradient-filled boxes. The twins of luce-browser-render's
// tests/raster/ladybird.lucb (same names); images under skia/ladybird/.
#pragma STDC FP_CONTRACT OFF
#include "scenes_ladybird.h"

#define LB(name) "skia/ladybird/" name ".png"

// mark: rounded rectangles -------------------------------------------------------------------

SCENE(ladybird_rrect_opaque_int) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    fill(pixmap, rounded_rect(10.0, 10.0, 80.0, 60.0, 10.0), solid(30, 90, 200, 255, true));
    save(pixmap, LB("rrect_opaque_int"));
}

SCENE(ladybird_rrect_translucent_frac) {
    Pixmap pixmap = lb_pixmap(100, 80, false);
    fill(pixmap, rounded_rect(10.3, 12.6, 70.5, 50.25, 8.5), solid(200, 50, 60, 150, true));
    save(pixmap, LB("rrect_translucent_frac"));
}

SCENE(ladybird_rrect_small_radius) {
    Pixmap pixmap = lb_pixmap(64, 48, true);
    fill(pixmap, rounded_rect(5.5, 6.5, 40.0, 20.0, 2.0), solid(30, 90, 200, 255, true));
    fill(pixmap, rounded_rect(8.25, 30.75, 50.0, 12.0, 1.5), solid(0, 0, 0, 180, true));
    save(pixmap, LB("rrect_small_radius"));
}

SCENE(ladybird_rrect_pill) {
    Pixmap pixmap = lb_pixmap(128, 64, true);
    fill(pixmap, rounded_rect(10.0, 17.0, 100.0, 30.0, 15.0), solid(80, 160, 70, 255, true));
    fill(pixmap, rounded_rect(14.4, 50.2, 60.5, 9.0, 4.5), solid(80, 160, 70, 200, true));
    save(pixmap, LB("rrect_pill"));
}

SCENE(ladybird_rrect_elliptical) {
    Pixmap pixmap = lb_pixmap(128, 80, false);
    fill(pixmap, elliptical_rect(8.0, 8.0, 110.0, 60.0, 30.0, 12.0), solid(120, 40, 160, 220, true));
    save(pixmap, LB("rrect_elliptical"));
}

SCENE(ladybird_rrect_tiny) {
    Pixmap pixmap = lb_pixmap(24, 24, true);
    fill(pixmap, rounded_rect(3.25, 4.75, 12.0, 10.0, 3.0), solid(30, 90, 200, 200, true));
    fill(pixmap, rounded_rect(10.5, 9.5, 9.0, 9.0, 4.5), solid(220, 120, 20, 160, true));
    save(pixmap, LB("rrect_tiny"));
}

SCENE(ladybird_rrect_mixed_corners) {
    Pixmap pixmap = lb_pixmap(120, 90, true);
    fill(pixmap, rounded_rect_radii(10.5, 10.5, 100.0, 70.0, {20.0, 20.0, 0.0, 0.0, 30.0, 15.0, 5.0, 40.0}), solid(40, 40, 40, 255, true));
    save(pixmap, LB("rrect_mixed_corners"));
}

SCENE(ladybird_rrect_non_aa) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    fill(pixmap, rounded_rect(10.3, 12.6, 70.5, 50.25, 12.0), solid(30, 90, 200, 255, false));
    save(pixmap, LB("rrect_non_aa"));
}

// mark: circles and ellipses -----------------------------------------------------------------

SCENE(ladybird_circle_small_radii) {
    Pixmap pixmap = lb_pixmap(96, 16, true);
    const float centers[6] = {6.3, 18.7, 31.1, 44.5, 60.2, 80.6};
    const float radii[6] = {0.5, 1.0, 1.5, 2.0, 3.0, 4.5};
    for (int i = 0; i < 6; i++) fill(pixmap, circle(centers[i], 8.4, radii[i]), solid(0, 0, 0, 255, true));
    save(pixmap, LB("circle_small_radii"));
}

SCENE(ladybird_circle_frac) {
    Pixmap pixmap = lb_pixmap(48, 48, false);
    fill(pixmap, circle(20.4, 20.6, 12.3), solid(30, 90, 200, 230, true));
    save(pixmap, LB("circle_frac"));
}

SCENE(ladybird_circle_large_translucent) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    fill(pixmap, circle(50.5, 50.25, 40.0), solid(200, 50, 60, 128, true));
    save(pixmap, LB("circle_large_translucent"));
}

SCENE(ladybird_circle_int_opaque) {
    Pixmap pixmap = lb_pixmap(64, 64, true);
    fill(pixmap, circle(32.0, 32.0, 20.0), solid(0, 0, 0, 255, true));
    save(pixmap, LB("circle_int_opaque"));
}

SCENE(ladybird_ellipse_wide) {
    Pixmap pixmap = lb_pixmap(120, 70, true);
    fill(pixmap, oval(10.5, 20.25, 100.0, 30.0), solid(80, 160, 70, 255, true));
    save(pixmap, LB("ellipse_wide"));
}

SCENE(ladybird_ellipse_tall) {
    Pixmap pixmap = lb_pixmap(60, 80, false);
    fill(pixmap, oval(20.7, 5.1, 18.6, 70.0), solid(120, 40, 160, 200, true));
    save(pixmap, LB("ellipse_tall"));
}

// 25 circles with radii from 0.5 to 38.9, overlapping.
SCENE(ladybird_circle_grid) {
    Pixmap pixmap = lb_pixmap(200, 200, true);
    Paint paint = solid(30, 90, 200, 110, true);
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 5; i++) {
            float r = 0.5f + (float)(j * 5 + i) * 1.6f;
            fill(pixmap, circle(20.3f + (float)i * 40.0f, 20.6f + (float)j * 40.0f, r), paint);
        }
    save(pixmap, LB("circle_grid"));
}

// mark: thin lines ---------------------------------------------------------------------------

// The line set offset by `o`, stroked `width` wide with `cap`.
static void lines_scene(const char* image, float width, bool aa, LineCap cap, float o) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    Stroke s;
    s.width = width;
    s.line_cap = cap;
    stroke(pixmap, line_set(o), solid(20, 20, 40, 220, aa), s);
    save(pixmap, image);
}

SCENE(ladybird_lines_w0_aa) { lines_scene(LB("lines_w0_aa"), 0.0, true, LineCap::Butt, 0.0); }
SCENE(ladybird_lines_w0_noaa) { lines_scene(LB("lines_w0_noaa"), 0.0, false, LineCap::Butt, 0.0); }
SCENE(ladybird_lines_w05_aa) { lines_scene(LB("lines_w05_aa"), 0.5, true, LineCap::Butt, 0.0); }
SCENE(ladybird_lines_w1_aa) { lines_scene(LB("lines_w1_aa"), 1.0, true, LineCap::Butt, 0.0); }
SCENE(ladybird_lines_w1_half_aa) { lines_scene(LB("lines_w1_half_aa"), 1.0, true, LineCap::Butt, 0.5); }
SCENE(ladybird_lines_w1_half_noaa) { lines_scene(LB("lines_w1_half_noaa"), 1.0, false, LineCap::Butt, 0.5); }
SCENE(ladybird_lines_w15_aa_round) { lines_scene(LB("lines_w15_aa_round"), 1.5, true, LineCap::Round, 0.25); }
SCENE(ladybird_lines_w2_aa_square) { lines_scene(LB("lines_w2_aa_square"), 2.0, true, LineCap::Square, 0.0); }
SCENE(ladybird_lines_w2_noaa_square) { lines_scene(LB("lines_w2_noaa_square"), 2.0, false, LineCap::Square, 0.5); }
SCENE(ladybird_lines_w3_half_aa_round) { lines_scene(LB("lines_w3_half_aa_round"), 3.0, true, LineCap::Round, 0.5); }

// mark: strokes ------------------------------------------------------------------------------

SCENE(ladybird_stroke_rect_w1_int) {
    Pixmap pixmap = lb_pixmap(80, 60, true);
    stroke(pixmap, rect_path(10.0, 10.0, 60.0, 40.0), solid(30, 90, 200, 255, true), Stroke());
    save(pixmap, LB("stroke_rect_w1_int"));
}

SCENE(ladybird_stroke_rect_w1_half) {
    Pixmap pixmap = lb_pixmap(80, 60, true);
    stroke(pixmap, rect_path(10.5, 10.5, 60.0, 40.0), solid(30, 90, 200, 255, true), Stroke());
    save(pixmap, LB("stroke_rect_w1_half"));
}

// The zigzag stroked 6 wide with `join`, translucent so overlaps would show.
static void polyline_scene(const char* image, LineJoin join) {
    Pixmap pixmap = lb_pixmap(120, 70, true);
    Stroke s;
    s.width = 6.0;
    s.line_join = join;
    stroke(pixmap, zigzag(), solid(200, 50, 60, 200, true), s);
    save(pixmap, image);
}

SCENE(ladybird_stroke_polyline_miter) { polyline_scene(LB("stroke_polyline_miter"), LineJoin::Miter); }
SCENE(ladybird_stroke_polyline_round) { polyline_scene(LB("stroke_polyline_round"), LineJoin::Round); }
SCENE(ladybird_stroke_polyline_bevel) { polyline_scene(LB("stroke_polyline_bevel"), LineJoin::Bevel); }

SCENE(ladybird_stroke_closed_w8_miter) {
    Pixmap pixmap = lb_pixmap(120, 80, true);
    Stroke s;
    s.width = 8.0;
    stroke(pixmap, triangle(15.0, 70.0, 60.0, 10.0, 105.0, 70.0), solid(30, 90, 200, 255, true), s);
    save(pixmap, LB("stroke_closed_w8_miter"));
}

SCENE(ladybird_stroke_dash_rect) {
    Pixmap pixmap = lb_pixmap(100, 70, true);
    Stroke s;
    s.width = 2.0;
    s.dash = stroke_dash({6.0, 3.0}, 0.0);
    stroke(pixmap, rect_path(10.5, 10.5, 80.0, 50.0), solid(40, 40, 40, 255, true), s);
    save(pixmap, LB("stroke_dash_rect"));
}

// Dotted borders: zero-length dashes with round caps.
SCENE(ladybird_stroke_dash_dotted) {
    Pixmap pixmap = lb_pixmap(120, 70, true);
    Paint paint = solid(40, 40, 40, 255, true);
    PathBuilder line;
    line.move_to(10.0, 20.0);
    line.line_to(110.0, 20.0);
    Stroke s;
    s.width = 4.0;
    s.line_cap = LineCap::Round;
    s.dash = stroke_dash({0.0, 8.0}, 0.0);
    stroke(pixmap, *line.finish(), paint, s);
    Stroke s2;
    s2.width = 2.0;
    s2.line_cap = LineCap::Round;
    s2.dash = stroke_dash({0.0, 6.0}, 0.0);
    stroke(pixmap, rect_path(10.0, 35.0, 100.0, 25.0), paint, s2);
    save(pixmap, LB("stroke_dash_dotted"));
}

SCENE(ladybird_stroke_dash_offset_polyline) {
    Pixmap pixmap = lb_pixmap(120, 70, true);
    Stroke s;
    s.width = 2.0;
    s.line_join = LineJoin::Round;
    s.dash = stroke_dash({10.0, 5.0, 2.0, 5.0}, 3.0);
    stroke(pixmap, zigzag(), solid(30, 90, 200, 255, true), s);
    save(pixmap, LB("stroke_dash_offset_polyline"));
}

SCENE(ladybird_stroke_rrect) {
    Pixmap pixmap = lb_pixmap(100, 70, true);
    stroke(pixmap, rounded_rect(10.5, 10.5, 80.0, 50.0, 10.0), solid(40, 40, 40, 255, true), Stroke());
    Stroke thick;
    thick.width = 3.0;
    stroke(pixmap, rounded_rect(20.0, 20.0, 60.0, 30.0, 8.0), solid(200, 50, 60, 180, true), thick);
    save(pixmap, LB("stroke_rrect"));
}

SCENE(ladybird_stroke_circles) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    Paint paint = solid(40, 40, 40, 255, true);
    stroke(pixmap, circle(25.5, 25.5, 15.0), paint, Stroke());
    Stroke s2;
    s2.width = 2.5;
    stroke(pixmap, circle(70.3, 25.7, 15.0), paint, s2);
    Stroke s3;
    s3.width = 0.0;
    stroke(pixmap, circle(47.5, 60.0, 12.0), paint, s3);
    save(pixmap, LB("stroke_circles"));
}

SCENE(ladybird_stroke_rrect_dashed) {
    Pixmap pixmap = lb_pixmap(120, 80, true);
    Stroke s;
    s.width = 2.0;
    s.dash = stroke_dash({8.0, 4.0}, 0.0);
    stroke(pixmap, rounded_rect(10.0, 10.0, 100.0, 60.0, 15.0), solid(30, 90, 200, 255, true), s);
    save(pixmap, LB("stroke_rrect_dashed"));
}

// mark: translucent overlaps -----------------------------------------------------------------

SCENE(ladybird_overlap_small_circles) {
    Pixmap pixmap = lb_pixmap(32, 32, true);
    fill(pixmap, circle(12.0, 12.0, 7.0), solid(220, 30, 30, 128, true));
    fill(pixmap, circle(19.5, 12.5, 7.0), solid(30, 180, 30, 128, true));
    fill(pixmap, circle(15.7, 18.9, 7.0), solid(30, 30, 220, 128, true));
    save(pixmap, LB("overlap_small_circles"));
}

SCENE(ladybird_overlap_tiny_rects) {
    Pixmap pixmap = lb_pixmap(24, 24, true);
    fill_rect(pixmap, rect_xywh(2.3, 3.7, 10.5, 8.2), solid(200, 50, 60, 140, true), identity(), nullptr);
    fill_rect(pixmap, rect_xywh(7.6, 6.1, 12.2, 9.9), solid(30, 90, 200, 140, true), identity(), nullptr);
    fill_rect(pixmap, rect_xywh(4.5, 12.25, 6.0, 6.0), solid(80, 160, 70, 180, true), identity(), nullptr);
    save(pixmap, LB("overlap_tiny_rects"));
}

// The background is a fill_rect, not Pixmap.fill.
SCENE(ladybird_overlap_large_circles) {
    Pixmap pixmap = lb_pixmap(200, 200, false);
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 200.0, 200.0), solid(250, 250, 245, 255, false), identity(), nullptr);
    fill(pixmap, circle(80.0, 80.0, 60.0), solid(220, 30, 30, 120, true));
    fill(pixmap, circle(120.5, 80.5, 60.0), solid(30, 180, 30, 120, true));
    fill(pixmap, circle(100.25, 115.75, 60.0), solid(30, 30, 220, 120, true));
    save(pixmap, LB("overlap_large_circles"));
}

SCENE(ladybird_overlap_triangles) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    fill(pixmap, triangle(10.0, 90.0, 50.5, 8.3, 90.0, 90.0), solid(200, 50, 60, 150, true));
    fill(pixmap, triangle(5.2, 30.4, 95.7, 45.1, 20.3, 85.9), solid(30, 90, 200, 150, true));
    fill(pixmap, triangle(30.0, 10.0, 70.0, 12.0, 52.0, 95.0), solid(0, 0, 0, 100, true));
    save(pixmap, LB("overlap_triangles"));
}

// Sixteen triangles 3 to 12 pixels wide at fractional positions.
SCENE(ladybird_overlap_tiny_shapes) {
    Pixmap pixmap = lb_pixmap(64, 64, true);
    Paint dark = solid(20, 20, 60, 160, true);
    Paint red = solid(200, 50, 60, 200, true);
    for (int i = 0; i < 16; i++) {
        float x = 2.3f + (float)(i % 4) * 15.1f;
        float y = 3.7f + (float)(i / 4) * 15.3f;
        float size = 3.0f + (float)i * 0.6f;
        SkPath path = triangle(x, y + size, x + size, y + size, x + size * 0.5f, y);
        fill(pixmap, path, i % 2 == 0 ? dark : red);
    }
    save(pixmap, LB("overlap_tiny_shapes"));
}

// mark: glyphs -------------------------------------------------------------------------------

// Twelve 14-pixel 'o's at fractional positions.
SCENE(ladybird_glyphs_o_row) {
    Pixmap pixmap = lb_pixmap(200, 32, false);
    pixmap_fill(pixmap, SkColors::kWhite);
    Paint paint = solid(0, 0, 0, 255, true);
    float x = 3.3f;
    for (int i = 0; i < 12; i++) {
        draw_glyph(pixmap, false, 0.7f, x, 8.35f, paint);
        x += 15.7f;
    }
    save(pixmap, LB("glyphs_o_row"));
}

// Two rows of 'o's and 'e's from 8 to 20 pixels tall on a common baseline.
SCENE(ladybird_glyphs_mixed_sizes) {
    Pixmap pixmap = lb_pixmap(200, 52, false);
    pixmap_fill(pixmap, SkColors::kWhite);
    Paint paint = solid(0, 0, 0, 255, true);
    for (int row = 0; row < 2; row++) {
        float x = 2.7f + (float)row * 0.45f;
        float baseline = 22.6f + (float)row * 26.0f;
        for (int i = 0; i < 7; i++) {
            float s = 0.4f + (float)i * 0.1f;
            draw_glyph(pixmap, (i + row) % 2 == 1, s, x, baseline - 20.0f * s, paint);
            x += 16.0f * s + 2.3f;
        }
    }
    save(pixmap, LB("glyphs_mixed_sizes"));
}

// Twenty translucent 9-pixel glyphs over the background.
SCENE(ladybird_glyphs_translucent_small) {
    Pixmap pixmap = lb_pixmap(160, 24, true);
    Paint paint = solid(20, 20, 60, 200, true);
    float x = 1.3f;
    for (int i = 0; i < 20; i++) {
        draw_glyph(pixmap, i % 3 == 0, 0.45f, x, 7.35f, paint);
        x += 7.9f;
    }
    save(pixmap, LB("glyphs_translucent_small"));
}

// mark: transforms ---------------------------------------------------------------------------

SCENE(ladybird_ts_rotate_rrect) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    fill_ts(pixmap, rounded_rect(-30.0, -20.0, 60.0, 40.0, 8.0), solid(30, 90, 200, 230, true), from_row(0.8660254, 0.5, -0.5, 0.8660254, 50.3, 50.1));
    save(pixmap, LB("ts_rotate_rrect"));
}

SCENE(ladybird_ts_scale_circle) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    fill_ts(pixmap, circle(0.0, 0.0, 10.0), solid(200, 50, 60, 200, true), from_row(2.5, 0.0, 0.0, 1.5, 50.5, 40.25));
    save(pixmap, LB("ts_scale_circle"));
}

SCENE(ladybird_ts_skew_rect) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    fill_ts(pixmap, rect_path(10.0, 10.0, 50.0, 40.0), solid(80, 160, 70, 200, true), from_row(1.0, 0.2, 0.4, 1.0, 5.0, 3.0));
    save(pixmap, LB("ts_skew_rect"));
}

SCENE(ladybird_ts_rotate_stroke_rrect) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    Stroke s;
    s.width = 2.0;
    stroke_path(pixmap, rounded_rect(-30.0, -18.0, 60.0, 36.0, 6.0), solid(40, 40, 40, 255, true), s,
                from_row(0.9659258, 0.258819, -0.258819, 0.9659258, 50.0, 40.0), nullptr);
    save(pixmap, LB("ts_rotate_stroke_rrect"));
}

// mark: gradients ----------------------------------------------------------------------------

SCENE(ladybird_grad_linear_rrect) {
    Pixmap pixmap = lb_pixmap(100, 80, false);
    Paint paint;
    paint.shader = linear_gradient({10.0, 10.0}, {90.0, 70.0}, {{0.0, rgba8(50, 127, 150, 255)}, {1.0, rgba8(220, 140, 75, 255)}}, SpreadMode::Pad, identity());
    fill(pixmap, rounded_rect(10.5, 10.5, 80.0, 60.0, 12.0), paint);
    save(pixmap, LB("grad_linear_rrect"));
}

SCENE(ladybird_grad_linear_circle_3stops) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    Paint paint;
    paint.shader = linear_gradient({10.0, 50.0}, {90.0, 50.0}, {{0.0, rgba8(220, 30, 30, 255)}, {0.5, rgba8(20, 200, 75, 200)}, {1.0, rgba8(30, 30, 220, 255)}}, SpreadMode::Pad, identity());
    fill(pixmap, circle(50.0, 50.0, 40.5), paint);
    save(pixmap, LB("grad_linear_circle_3stops"));
}

SCENE(ladybird_grad_radial_circle) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    Paint paint;
    paint.shader = radial_gradient({50.5, 50.5}, 0.0, {50.5, 50.5}, 40.0, {{0.0, rgba8(255, 255, 255, 255)}, {1.0, rgba8(30, 90, 200, 255)}}, SpreadMode::Pad, identity());
    fill(pixmap, circle(50.5, 50.5, 40.0), paint);
    save(pixmap, LB("grad_radial_circle"));
}

SCENE(ladybird_grad_radial_rrect_3stops) {
    Pixmap pixmap = lb_pixmap(120, 80, true);
    Paint paint;
    paint.shader = radial_gradient({60.0, 40.0}, 0.0, {60.0, 40.0}, 50.0, {{0.0, rgba8(250, 220, 50, 255)}, {0.6, rgba8(220, 80, 40, 200)}, {1.0, rgba8(60, 20, 120, 160)}}, SpreadMode::Pad, identity());
    fill(pixmap, rounded_rect(10.0, 10.0, 100.0, 60.0, 20.0), paint);
    save(pixmap, LB("grad_radial_rrect_3stops"));
}
