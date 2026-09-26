// scenes_mask.cpp - tiny-skia tests/integration/mask.rs, rendered with Skia m144.
//
// A Mask built from paths is a Skia clip (see Mask in oracle.h). apply_mask, mask_from_alpha
// and mask_from_luma have no Skia call: they are tiny-skia's pixel formulas run in C++ on
// Skia-rendered pixels.
#include "oracle.h"

SCENE(mask_rect) {
    SkPath clip_path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, false, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Pixmap pixmap = new_pixmap(100, 100);
    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);
    fill_rect(pixmap, rect, paint, identity(), &mask);

    save(pixmap, "mask/rect.png");
}

SCENE(mask_rect_aa) {
    SkPath clip_path = path_from_rect(rect_xywh(10.5, 10.0, 80.0, 80.5));
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, true, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Pixmap pixmap = new_pixmap(100, 100);
    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);
    fill_rect(pixmap, rect, paint, identity(), &mask);

    save(pixmap, "mask/rect-aa.png");
}

SCENE(mask_rect_ts) {
    Pixmap pixmap = new_pixmap(100, 100);

    SkPath clip_path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));
    clip_path = clip_path.makeTransform(from_row(1.0, -0.3, 0.0, 1.0, 0.0, 15.0));

    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, false, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);
    fill_rect(pixmap, rect, paint, identity(), &mask);

    save(pixmap, "mask/rect-ts.png");
}

SCENE(mask_circle_bottom_right_aa) {
    Pixmap pixmap = new_pixmap(100, 100);

    SkPath clip_path = *path_from_circle(100.0, 100.0, 50.0);
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, true, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);
    fill_rect(pixmap, rect, paint, identity(), &mask);

    save(pixmap, "mask/circle-bottom-right-aa.png");
}

SCENE(mask_stroke) {
    Pixmap pixmap = new_pixmap(100, 100);

    SkPath clip_path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, false, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Stroke stroke;
    stroke.width = 10.0;

    SkPath path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));
    stroke_path(pixmap, path, paint, stroke, identity(), &mask);

    save(pixmap, "mask/stroke.png");
}

// Make sure we're clipping only source and not source and destination
SCENE(mask_skip_dest) {
    Pixmap pixmap = new_pixmap(100, 100);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    fill_path(pixmap, path_from_rect(rect_xywh(5.0, 5.0, 60.0, 60.0)), paint, FillRule::Winding, identity(), nullptr);

    Pixmap pixmap2 = new_pixmap(200, 200);
    fill_path(pixmap2, path_from_rect(rect_xywh(35.0, 35.0, 60.0, 60.0)), paint, FillRule::Winding, identity(), nullptr);

    SkPath clip_path = path_from_rect(rect_xywh(40.0, 40.0, 40.0, 40.0));
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, true, identity());

    draw_pixmap(pixmap, 0, 0, pixmap2, PixmapPaint{}, identity(), &mask);

    save(pixmap, "mask/skip-dest.png");
}

SCENE(mask_intersect_aa) {
    SkPath circle1 = *path_from_circle(75.0, 75.0, 50.0);
    SkPath circle2 = *path_from_circle(125.0, 125.0, 50.0);

    Mask mask = new_mask(200, 200);
    mask.fill_path(circle1, FillRule::Winding, true, identity());
    mask.intersect_path(circle2, FillRule::Winding, true, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Pixmap pixmap = new_pixmap(200, 200);
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 200.0, 200.0), paint, identity(), &mask);

    save(pixmap, "mask/intersect-aa.png");
}

SCENE(mask_ignore_memset) {
    SkPath clip_path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));

    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, false, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 255);
    paint.anti_alias = false;

    Pixmap pixmap = new_pixmap(100, 100);
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 100.0, 100.0), paint, identity(), &mask);

    save(pixmap, "mask/ignore-memset.png");
}

SCENE(mask_ignore_source) {
    SkPath clip_path = path_from_rect(rect_xywh(10.0, 10.0, 80.0, 80.0));

    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, false, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 255); // Must be opaque.
    paint.blend_mode = SkBlendMode::kSrcOver;
    paint.anti_alias = false;

    Pixmap pixmap = new_pixmap(100, 100);
    pixmap_fill(pixmap, SkColors::kWhite);
    fill_rect(pixmap, rect_xywh(0.0, 0.0, 100.0, 100.0), paint, identity(), &mask);

    save(pixmap, "mask/ignore-source.png");
}

SCENE(mask_apply_mask) {
    Pixmap pixmap = new_pixmap(100, 100);

    SkPath clip_path = *path_from_circle(100.0, 100.0, 50.0);
    Mask mask = new_mask(100, 100);
    mask.fill_path(clip_path, FillRule::Winding, true, identity());

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    SkRect rect = rect_xywh(0.0, 0.0, 100.0, 100.0);
    fill_rect(pixmap, rect, paint, identity(), nullptr);
    // not a Skia operation: the mask's Skia coverage, applied with tiny-skia's lowp
    // destination_in in C++.
    apply_mask(pixmap, mask.coverage());

    save(pixmap, "mask/apply-mask.png");
}

SCENE(mask_mask_from_alpha) {
    SkPath path = *path_from_circle(100.0, 100.0, 50.0);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    // not a Skia operation: tiny-skia's Mask::from_pixmap(Alpha) on the Skia pixels.
    std::vector<uint8_t> mask = mask_from_pixmap(pixmap, MaskType::Alpha);

    save_mask(mask, 100, 100, "mask/mask-from-alpha.png");
}

SCENE(mask_mask_from_luma) {
    SkPath path = *path_from_circle(100.0, 100.0, 50.0);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    // not a Skia operation: tiny-skia's Mask::from_pixmap(Luminance) on the Skia pixels.
    std::vector<uint8_t> mask = mask_from_pixmap(pixmap, MaskType::Luminance);

    save_mask(mask, 100, 100, "mask/mask-from-luma.png");
}
