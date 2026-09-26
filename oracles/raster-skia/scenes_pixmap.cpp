// scenes_pixmap.cpp - tiny-skia tests/integration/pixmap.rs, rendered with Skia m144.
//
// clone_rect_out_of_bound only asserts is_none and fill only checks Pixmap::fill (a raw
// store, no Skia drawing): no scene.
#include "oracle.h"

static Pixmap circle_pixmap() {
    Pixmap pixmap = new_pixmap(200, 200);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    fill_path(pixmap, *path_from_circle(100.0, 100.0, 80.0), paint, FillRule::Winding, identity(), nullptr);
    return pixmap;
}

SCENE(pixmap_clone_rect_1) {
    Pixmap pixmap = circle_pixmap();
    Pixmap part = *clone_rect(pixmap, 10, 15, 80, 90);
    save(part, "pixmap/clone-rect-1.png");
}

SCENE(pixmap_clone_rect_2) {
    Pixmap pixmap = circle_pixmap();
    Pixmap part = *clone_rect(pixmap, 130, 120, 80, 90);
    save(part, "pixmap/clone-rect-2.png");
}

// Tests that painting algorithm will switch `Bicubic`/`Bilinear` to `Nearest`.
// Otherwise we will get a blurry image.
SCENE(pixmap_draw_pixmap) {
    // A pixmap with the bottom half filled with solid color.
    Pixmap sub_pixmap = [] {
        Paint paint;
        paint.set_color_rgba8(50, 127, 150, 200);
        paint.anti_alias = false;

        SkRect rect = rect_xywh(0.0, 50.0, 100.0, 50.0);

        Pixmap pixmap = new_pixmap(100, 100);
        fill_rect(pixmap, rect, paint, identity(), nullptr);
        return pixmap;
    }();

    PixmapPaint paint;
    paint.quality = FilterQuality::Bicubic;

    Pixmap pixmap = new_pixmap(200, 200);
    draw_pixmap(pixmap, 20, 20, sub_pixmap, paint, identity(), nullptr);

    save(pixmap, "canvas/draw-pixmap.png");
}

static Pixmap triangle_100() {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(0.0, 100.0);
    pb.line_to(100.0, 100.0);
    pb.line_to(50.0, 0.0);
    pb.close();
    SkPath path = *pb.finish();

    Pixmap pixmap = new_pixmap(100, 100);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);
    return pixmap;
}

SCENE(pixmap_draw_pixmap_ts) {
    Pixmap triangle = triangle_100();

    PixmapPaint paint;
    paint.quality = FilterQuality::Bicubic;

    Pixmap pixmap = new_pixmap(200, 200);
    draw_pixmap(pixmap, 5, 10, triangle, paint, from_row(1.2, 0.5, 0.5, 1.2, 0.0, 0.0), nullptr);

    save(pixmap, "canvas/draw-pixmap-ts.png");
}

SCENE(pixmap_draw_pixmap_opacity) {
    Pixmap triangle = triangle_100();

    PixmapPaint paint;
    paint.quality = FilterQuality::Bicubic;
    paint.opacity = 0.5;

    Pixmap pixmap = new_pixmap(200, 200);
    draw_pixmap(pixmap, 5, 10, triangle, paint, from_row(1.2, 0.5, 0.5, 1.2, 0.0, 0.0), nullptr);

    save(pixmap, "canvas/draw-pixmap-opacity.png");
}
