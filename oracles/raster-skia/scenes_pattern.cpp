// scenes_pattern.cpp - tiny-skia tests/integration/pattern.rs, rendered with Skia m144.
#include "oracle.h"

static Pixmap crate_triangle() {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(0.0, 20.0);
    pb.line_to(20.0, 20.0);
    pb.line_to(10.0, 0.0);
    pb.close();
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(20, 20);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);
    return pixmap;
}

// Every pattern test: the triangle as a pattern (opacity 1.0) filling rect 10..190 of a
// 200x200 pixmap.
static void pattern_scene(SpreadMode mode, FilterQuality quality, const SkMatrix& ts, const char* image) {
    Pixmap triangle = crate_triangle();

    Paint paint;
    paint.anti_alias = false;
    paint.shader = pattern(triangle, mode, quality, ts);
    paint.shader_opacity = 1.0;

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, image);
}

SCENE(pattern_pad_nearest) { pattern_scene(SpreadMode::Pad, FilterQuality::Nearest, identity(), "pattern/pad-nearest.png"); }
SCENE(pattern_repeat_nearest) { pattern_scene(SpreadMode::Repeat, FilterQuality::Nearest, identity(), "pattern/repeat-nearest.png"); }
SCENE(pattern_reflect_nearest) { pattern_scene(SpreadMode::Reflect, FilterQuality::Nearest, identity(), "pattern/reflect-nearest.png"); }

// We have to test tile mode for bilinear/bicubic separately,
// because they're using a different algorithm from nearest.
// (Transform must be set, otherwise tiny-skia falls back to Nearest.)
SCENE(pattern_pad_bicubic) { pattern_scene(SpreadMode::Pad, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "pattern/pad-bicubic.png"); }
SCENE(pattern_repeat_bicubic) { pattern_scene(SpreadMode::Repeat, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "pattern/repeat-bicubic.png"); }
SCENE(pattern_reflect_bicubic) { pattern_scene(SpreadMode::Reflect, FilterQuality::Bicubic, from_row(1.1, 0.3, 0.0, 1.4, 0.0, 0.0), "pattern/reflect-bicubic.png"); }

SCENE(pattern_filter_nearest_no_ts) { pattern_scene(SpreadMode::Repeat, FilterQuality::Nearest, identity(), "pattern/filter-nearest-no-ts.png"); }
SCENE(pattern_filter_nearest) { pattern_scene(SpreadMode::Repeat, FilterQuality::Nearest, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "pattern/filter-nearest.png"); }
SCENE(pattern_filter_bilinear) { pattern_scene(SpreadMode::Repeat, FilterQuality::Bilinear, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "pattern/filter-bilinear.png"); }
SCENE(pattern_filter_bicubic) { pattern_scene(SpreadMode::Repeat, FilterQuality::Bicubic, from_row(1.5, 0.0, -0.4, -0.8, 5.0, 1.0), "pattern/filter-bicubic.png"); }
