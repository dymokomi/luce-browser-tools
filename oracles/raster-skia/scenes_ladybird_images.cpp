// scenes_ladybird_images.cpp - Ladybird-style scenes: images drawn with draw_pixmap
// (offsets, scales, rotation, filter qualities, opacity), pattern fills and clips (masks).
// The twins of luce-browser-render's tests/raster/ladybird.lucb (same names); images under
// skia/ladybird/.
#pragma STDC FP_CONTRACT OFF
#include "scenes_ladybird.h"

#define LB(name) "skia/ladybird/" name ".png"

// mark: images -------------------------------------------------------------------------------

// The 16x16 test image drawn into a `size` x `size` background at (x, y) under `ts`.
static void image_scene(const char* image, int size, int x, int y, FilterQuality quality, float opacity, const SkMatrix& ts) {
    Pixmap source = source_image(16);
    Pixmap pixmap = lb_pixmap(size, size, true);
    PixmapPaint paint;
    paint.quality = quality;
    paint.opacity = opacity;
    draw_pixmap(pixmap, x, y, source, paint, ts, nullptr);
    save(pixmap, image);
}

SCENE(ladybird_image_int) { image_scene(LB("image_int"), 48, 10, 12, FilterQuality::Nearest, 1.0, identity()); }
SCENE(ladybird_image_frac_nearest) { image_scene(LB("image_frac_nearest"), 48, 0, 0, FilterQuality::Nearest, 1.0, from_translate(10.5, 12.25)); }
SCENE(ladybird_image_frac_bilinear) { image_scene(LB("image_frac_bilinear"), 48, 0, 0, FilterQuality::Bilinear, 1.0, from_translate(10.5, 12.25)); }
SCENE(ladybird_image_scale2_nearest) { image_scene(LB("image_scale2_nearest"), 48, 0, 0, FilterQuality::Nearest, 1.0, from_row(2.0, 0.0, 0.0, 2.0, 7.0, 5.0)); }
SCENE(ladybird_image_scale2_bilinear) { image_scene(LB("image_scale2_bilinear"), 48, 0, 0, FilterQuality::Bilinear, 1.0, from_row(2.0, 0.0, 0.0, 2.0, 7.0, 5.0)); }
SCENE(ladybird_image_scale2_bicubic) { image_scene(LB("image_scale2_bicubic"), 48, 0, 0, FilterQuality::Bicubic, 1.0, from_row(2.0, 0.0, 0.0, 2.0, 7.0, 5.0)); }
SCENE(ladybird_image_rotate_bilinear) { image_scene(LB("image_rotate_bilinear"), 48, 0, 0, FilterQuality::Bilinear, 1.0, from_row(0.8660254, 0.5, -0.5, 0.8660254, 20.0, 6.0)); }
SCENE(ladybird_image_opacity_half) { image_scene(LB("image_opacity_half"), 48, 10, 12, FilterQuality::Nearest, 0.5, identity()); }
SCENE(ladybird_image_opacity_half_bicubic) { image_scene(LB("image_opacity_half_bicubic"), 48, 0, 0, FilterQuality::Bicubic, 0.5, from_row(1.5, 0.0, 0.0, 1.5, 5.3, 6.6)); }

// A 32x32 image drawn at half size.
SCENE(ladybird_image_scale_half_bilinear) {
    Pixmap source = source_image(32);
    Pixmap pixmap = lb_pixmap(32, 32, true);
    PixmapPaint paint;
    paint.quality = FilterQuality::Bilinear;
    draw_pixmap(pixmap, 0, 0, source, paint, from_row(0.5, 0.0, 0.0, 0.5, 8.25, 8.75), nullptr);
    save(pixmap, LB("image_scale_half_bilinear"));
}

SCENE(ladybird_pattern_repeat_nearest) {
    Pixmap source = source_image(16);
    Pixmap pixmap = lb_pixmap(64, 64, true);
    Paint paint;
    paint.anti_alias = false;
    paint.shader = pattern(source, SpreadMode::Repeat, FilterQuality::Nearest, from_translate(3.0, 5.0));
    paint.shader_opacity = 1.0;
    fill_rect(pixmap, rect_xywh(4.0, 4.0, 56.0, 56.0), paint, identity(), nullptr);
    save(pixmap, LB("pattern_repeat_nearest"));
}

SCENE(ladybird_pattern_pad_bilinear_rrect) {
    Pixmap source = source_image(16);
    Pixmap pixmap = lb_pixmap(80, 80, true);
    Paint paint;
    paint.shader = pattern(source, SpreadMode::Pad, FilterQuality::Bilinear, from_row(3.0, 0.0, 0.0, 3.0, 10.0, 10.0));
    paint.shader_opacity = 1.0;
    fill(pixmap, rounded_rect(8.5, 8.5, 60.0, 60.0, 12.0), paint);
    save(pixmap, LB("pattern_pad_bilinear_rrect"));
}

// mark: clips --------------------------------------------------------------------------------

SCENE(ladybird_clip_rrect_fill) {
    Pixmap pixmap = lb_pixmap(100, 80, true);
    Mask mask = new_mask(100, 80);
    mask.fill_path(rounded_rect(10.5, 10.25, 80.0, 60.0, 14.0), FillRule::Winding, true, identity());
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 100.0, 80.0), solid(30, 90, 200, 255, false), identity(), &mask);
    save(pixmap, LB("clip_rrect_fill"));
}

SCENE(ladybird_clip_circle_stroke) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    Mask mask = new_mask(100, 100);
    mask.fill_path(circle(50.3, 50.6, 35.0), FillRule::Winding, true, identity());
    Stroke s;
    s.width = 3.0;
    stroke_path(pixmap, line_set(0.0), solid(200, 50, 60, 255, true), s, identity(), &mask);
    save(pixmap, LB("clip_circle_stroke"));
}

SCENE(ladybird_clip_circle_image) {
    Pixmap source = source_image(16);
    Pixmap pixmap = lb_pixmap(64, 64, true);
    Mask mask = new_mask(64, 64);
    mask.fill_path(circle(32.5, 32.5, 24.0), FillRule::Winding, true, identity());
    PixmapPaint paint;
    paint.quality = FilterQuality::Bilinear;
    draw_pixmap(pixmap, 0, 0, source, paint, from_row(3.5, 0.0, 0.0, 3.5, 4.0, 4.0), &mask);
    save(pixmap, LB("clip_circle_image"));
}

SCENE(ladybird_clip_polygon_noaa) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    PathBuilder pb;
    pb.move_to(50.0, 5.0);
    pb.line_to(61.0, 38.0);
    pb.line_to(95.0, 38.0);
    pb.line_to(67.0, 58.0);
    pb.line_to(78.0, 92.0);
    pb.line_to(50.0, 72.0);
    pb.line_to(22.0, 92.0);
    pb.line_to(33.0, 58.0);
    pb.line_to(5.0, 38.0);
    pb.line_to(39.0, 38.0);
    pb.close();
    Mask mask = new_mask(100, 100);
    mask.fill_path(*pb.finish(), FillRule::Winding, false, identity());
    fill_path(pixmap, circle(50.0, 50.0, 45.0), solid(80, 160, 70, 200, true), FillRule::Winding, identity(), &mask);
    save(pixmap, LB("clip_polygon_noaa"));
}

SCENE(ladybird_clip_intersect) {
    Pixmap pixmap = lb_pixmap(100, 100, true);
    Mask mask = new_mask(100, 100);
    mask.fill_path(circle(40.0, 50.0, 35.0), FillRule::Winding, true, identity());
    mask.intersect_path(rounded_rect(30.5, 20.5, 60.0, 60.0, 10.0), FillRule::Winding, true, identity());
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 100.0, 100.0), solid(120, 40, 160, 255, false), identity(), &mask);
    save(pixmap, LB("clip_intersect"));
}

SCENE(ladybird_clip_pixel_rect) {
    Pixmap pixmap = lb_pixmap(64, 64, true);
    Mask mask = new_mask(64, 64);
    mask.fill_path(rect_path(8.0, 8.0, 48.0, 40.0), FillRule::Winding, false, identity());
    fill_path(pixmap, circle(32.3, 30.7, 26.0), solid(120, 40, 160, 230, true), FillRule::Winding, identity(), &mask);
    save(pixmap, LB("clip_pixel_rect"));
}
