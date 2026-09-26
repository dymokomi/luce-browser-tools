// scenes_fill.cpp - tiny-skia tests/integration/fill.rs, rendered with Skia m144.
//
// fill/empty.png is written by three scenes (horizontal_line, vertical_line, single_line) and
// fill/polygon.png by two (open_polygon, closed_polygon), as in the Rust tests; the last one
// run wins. "Must not panic" tests and pixel-array tests are saved under extra/.
#include "oracle.h"

SCENE(fill_horizontal_line) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 10.0);
    pb.line_to(90.0, 10.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/empty.png");
}

SCENE(fill_vertical_line) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 10.0);
    pb.line_to(10.0, 90.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/empty.png");
}

SCENE(fill_single_line) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 10.0);
    pb.line_to(90.0, 90.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/empty.png");
}

SCENE(fill_int_rect) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(10.0, 15.0, 80.0, 70.0);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/int-rect.png");
}

SCENE(fill_float_rect) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(10.3, 15.4, 80.5, 70.6);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect.png");
}

SCENE(fill_int_rect_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(10.0, 15.0, 80.0, 70.0);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/int-rect-aa.png");
}

SCENE(fill_float_rect_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(10.3, 15.4, 80.5, 70.6);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect-aa.png");
}

SCENE(fill_float_rect_aa_highp) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;
    // paint.force_hq_pipeline = true: no Skia equivalent

    SkRect rect = rect_xywh(10.3, 15.4, 80.5, 70.6);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect-aa-highp.png");
}

SCENE(fill_tiny_float_rect) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(1.3, 1.4, 0.5, 0.6);
    Pixmap pixmap = new_pixmap(3, 3);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    check_pixels(pixmap, {
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
        {0, 0, 0, 0}, {50, 127, 150, 200}, {0, 0, 0, 0},
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    }, "fill_tiny_float_rect");
    save(pixmap, "extra/fill-tiny-float-rect.png");
}

SCENE(fill_tiny_float_rect_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(1.3, 1.4, 0.5, 0.6);

    Pixmap pixmap = new_pixmap(3, 3);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    check_pixels(pixmap, {
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
        {0, 0, 0, 0}, {51, 128, 153, 60}, {0, 0, 0, 0},
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    }, "fill_tiny_float_rect_aa");
    save(pixmap, "extra/fill-tiny-float-rect-aa.png");
}

// Must not panic (no expected image).
SCENE(fill_tiny_rect_aa) {
    Paint paint;
    paint.set_color_rgba8(0, 0, 0, 0);
    paint.anti_alias = true;
    SkRect rect = rect_xywh(0.7, 0.0, 1.0, 2.0);
    Pixmap pixmap = new_pixmap(10, 10);
    fill_rect(pixmap, rect, paint, identity(), nullptr);
    save(pixmap, "extra/fill-tiny-rect-aa.png");
}

SCENE(fill_float_rect_clip_top_left_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(-10.3, -20.4, 100.5, 70.2);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect-clip-top-left-aa.png");
}

SCENE(fill_float_rect_clip_top_right_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(60.3, -20.4, 100.5, 70.2);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect-clip-top-right-aa.png");
}

SCENE(fill_float_rect_clip_bottom_right_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkRect rect = rect_xywh(60.3, 40.4, 100.5, 70.2);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, identity(), nullptr);

    save(pixmap, "fill/float-rect-clip-bottom-right-aa.png");
}

SCENE(fill_int_rect_with_ts_clip_right) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect, paint, from_row(1.0, 0.0, 0.0, 1.0, 0.5, 0.5), nullptr);

    save(pixmap, "fill/int-rect-with-ts-clip-right.png");
}

SCENE(fill_open_polygon) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(75.160671, 88.756136);
    pb.line_to(24.797274, 88.734053);
    pb.line_to( 9.255130, 40.828792);
    pb.line_to(50.012955, 11.243795);
    pb.line_to(90.744819, 40.864522);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/polygon.png");
}

// Must be the same a open.
SCENE(fill_closed_polygon) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(75.160671, 88.756136);
    pb.line_to(24.797274, 88.734053);
    pb.line_to( 9.255130, 40.828792);
    pb.line_to(50.012955, 11.243795);
    pb.line_to(90.744819, 40.864522);
    pb.close(); // the only difference
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/polygon.png");
}

SCENE(fill_winding_star) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(50.0,  7.5);
    pb.line_to(75.0, 87.5);
    pb.line_to(10.0, 37.5);
    pb.line_to(90.0, 37.5);
    pb.line_to(25.0, 87.5);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/winding-star.png");
}

SCENE(fill_even_odd_star) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(50.0,  7.5);
    pb.line_to(75.0, 87.5);
    pb.line_to(10.0, 37.5);
    pb.line_to(90.0, 37.5);
    pb.line_to(25.0, 87.5);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::EvenOdd, identity(), nullptr);

    save(pixmap, "fill/even-odd-star.png");
}

SCENE(fill_quad_curve) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 15.0);
    pb.quad_to(95.0, 35.0, 75.0, 90.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::EvenOdd, identity(), nullptr);

    save(pixmap, "fill/quad.png");
}

SCENE(fill_cubic_curve) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 15.0);
    pb.cubic_to(95.0, 35.0, 0.0, 75.0, 75.0, 90.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::EvenOdd, identity(), nullptr);

    save(pixmap, "fill/cubic.png");
}

SCENE(fill_memset2d) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 255); // Must be opaque to trigger memset2d.
    paint.anti_alias = false;

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 90.0, 90.0));

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/memset2d.png");
}

// Make sure we do not write past pixmap memory.
SCENE(fill_memset2d_out_of_bounds) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 255); // Must be opaque to trigger memset2d.
    paint.anti_alias = false;

    SkPath path = path_from_rect(rect_ltrb(50.0, 50.0, 120.0, 120.0));

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/memset2d-2.png");
}

SCENE(fill_fill_aa) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(50.0,  7.5);
    pb.line_to(75.0, 87.5);
    pb.line_to(10.0, 37.5);
    pb.line_to(90.0, 37.5);
    pb.line_to(25.0, 87.5);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::EvenOdd, identity(), nullptr);

    save(pixmap, "fill/star-aa.png");
}

// Must not panic (no expected image).
SCENE(fill_overflow_in_walk_edges_1) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 20.0);
    pb.cubic_to(39.0, 163.0, 117.0, 61.0, 130.0, 70.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);
    save(pixmap, "extra/fill-overflow-in-walk-edges-1.png");
}

SCENE(fill_clip_line_1) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(50.0, -15.0);
    pb.line_to(-15.0, 50.0);
    pb.line_to(50.0, 115.0);
    pb.line_to(115.0, 50.0);
    pb.close();
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clip-line-1.png");
}

SCENE(fill_clip_line_2) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    // This strange path forces `line_clipper::clip` to return an empty array.
    PathBuilder pb;
    pb.move_to(0.0, -1.0);
    pb.line_to(50.0, 0.0);
    pb.line_to(0.0, 50.0);
    pb.close();
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clip-line-2.png");
}

SCENE(fill_clip_quad) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    PathBuilder pb;
    pb.move_to(10.0, 85.0);
    pb.quad_to(150.0, 150.0, 85.0, 15.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clip-quad.png");
}

SCENE(fill_clip_cubic_1) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    // `line_clipper::clip` produces 2 points for this path.
    PathBuilder pb;
    pb.move_to(10.0, 50.0);
    pb.cubic_to(0.0, 175.0, 195.0, 70.0, 75.0, 20.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clip-cubic-1.png");
}

SCENE(fill_clip_cubic_2) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    // `line_clipper::clip` produces 3 points for this path.
    PathBuilder pb;
    pb.move_to(10.0, 50.0);
    pb.cubic_to(10.0, 40.0, 90.0, 120.0, 125.0, 20.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clip-cubic-2.png");
}

// Must not loop (no expected image).
SCENE(fill_aa_endless_loop) {
    Paint paint;
    paint.anti_alias = true;

    // This path was causing an endless loop before.
    PathBuilder pb;
    pb.move_to(2.1537175, 11.560721);
    pb.quad_to(1.9999998, 10.787931, 2.0, 10.0);
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);
    save(pixmap, "extra/fill-aa-endless-loop.png");
}

SCENE(fill_clear_aa) {
    // Make sure that Clear with AA doesn't fallback to memset.
    Paint paint;
    paint.anti_alias = true;
    paint.blend_mode = SkBlendMode::kClear;

    Pixmap pixmap = new_pixmap(100, 100);
    pixmap_fill(pixmap, rgba8(50, 127, 150, 200));
    fill_path(pixmap, *path_from_circle(50.0, 50.0, 40.0), paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "fill/clear-aa.png");
}

// Must not panic (no expected image).
SCENE(fill_line_curve) {
    Paint paint;
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(100.0, 20.0);
    pb.cubic_to(100.0, 40.0, 100.0, 160.0, 100.0, 180.0); // Just a line.
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);
    save(pixmap, "extra/fill-line-curve.png");
}

SCENE(fill_vertical_lines_merging_bug) {
    // This path must not trigger edge_builder::combine_vertical,
    // otherwise AlphaRuns::add will crash later.
    PathBuilder pb;
    pb.move_to(765.56, 158.56);
    pb.line_to(754.4, 168.28);
    pb.cubic_to(754.4, 168.28, 754.4, 168.24, 754.4, 168.17);
    pb.cubic_to(754.4, 168.09, 754.4, 168.02, 754.4, 167.95);
    pb.line_to(754.4, 168.06);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, from_row(5.4, 0.0, 0.0, 5.4, -4050.0, -840.0), nullptr);

    save(pixmap, "fill/vertical-lines-merging-bug.png");
}

SCENE(fill_fill_rect) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect_xywh(20.3, 10.4, 50.5, 30.2), paint, from_row(1.2, 0.3, -0.7, 0.8, 12.0, 15.3), nullptr);

    save(pixmap, "canvas/fill-rect.png");
}
