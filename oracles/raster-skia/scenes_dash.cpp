// scenes_dash.cpp - tiny-skia tests/integration/dash.rs, rendered with Skia m144.
#include "oracle.h"

SCENE(dash_line) {
    PathBuilder pb;
    pb.move_to(10.0, 20.0);
    pb.line_to(90.0, 80.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Stroke stroke;
    stroke.dash = stroke_dash({5.0, 10.0}, 0.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/line.png");
}

SCENE(dash_quad) {
    PathBuilder pb;
    pb.move_to(10.0, 20.0);
    pb.quad_to(35.0, 75.0, 90.0, 80.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Stroke stroke;
    stroke.dash = stroke_dash({5.0, 10.0}, 0.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/quad.png");
}

SCENE(dash_cubic) {
    PathBuilder pb;
    pb.move_to(10.0, 20.0);
    pb.cubic_to(95.0, 35.0, 0.0, 75.0, 75.0, 90.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Stroke stroke;
    stroke.dash = stroke_dash({5.0, 10.0}, 0.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/cubic.png");
}

SCENE(dash_hairline) {
    PathBuilder pb;
    pb.move_to(10.0, 20.0);
    pb.cubic_to(95.0, 35.0, 0.0, 75.0, 75.0, 90.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.dash = stroke_dash({5.0, 10.0}, 0.0);
    stroke.width = 0.5;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/hairline.png");
}

SCENE(dash_complex) {
    PathBuilder pb;
    pb.move_to(28.7, 23.9);
    pb.line_to(177.4, 35.2);
    pb.line_to(177.4, 68.0);
    pb.line_to(129.7, 68.0);
    pb.cubic_to(81.6, 59.3, 41.8, 63.3, 33.4, 115.2);
    pb.cubic_to(56.8, 128.7, 77.3, 143.8, 53.3, 183.8);
    pb.cubic_to(113.8, 185.7, 91.0, 109.7, 167.3, 111.8);
    pb.cubic_to(-56.2, 90.3, 177.3, 68.0, 110.2, 95.5);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.dash = stroke_dash({10.0, 5.0}, 2.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(200, 200);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/complex.png");
}

SCENE(dash_multi_subpaths) {
    PathBuilder pb;
    pb.move_to(49.0, 76.0);
    pb.cubic_to(22.0, 150.0, 11.0, 213.0, 186.0, 151.0);
    pb.cubic_to(194.0, 106.0, 195.0, 64.0, 169.0, 26.0);
    pb.move_to(124.0, 41.0);
    pb.line_to(162.0, 105.0);
    pb.cubic_to(135.0, 175.0, 97.0, 166.0, 53.0, 128.0);
    pb.line_to(93.0, 71.0);
    pb.move_to(24.0, 52.0);
    pb.line_to(108.0, 20.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.dash = stroke_dash({10.0, 5.0}, 2.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(200, 200);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/multi_subpaths.png");
}

SCENE(dash_closed) {
    PathBuilder pb;
    pb.move_to(22.0, 22.0);
    pb.cubic_to(63.0, 16.0, 82.0, 24.0, 84.0, 46.0);
    pb.cubic_to(86.0, 73.0, 15.0, 58.0, 16.0, 89.0);
    pb.close();
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.dash = stroke_dash({10.0, 5.0}, 2.0);
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "dash/closed.png");
}
