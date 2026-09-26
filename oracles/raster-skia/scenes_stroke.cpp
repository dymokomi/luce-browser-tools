// scenes_stroke.cpp - tiny-skia tests/integration/stroke.rs, rendered with Skia m144.
#include "oracle.h"

SCENE(stroke_round_caps_and_large_scale) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(60.0f / 16.0f, 100.0f / 16.0f);
    pb.line_to(140.0f / 16.0f, 100.0f / 16.0f);
    SkPath path = *pb.finish();

    Stroke stroke;
    stroke.width = 6.0;
    stroke.line_cap = LineCap::Round;

    SkMatrix transform = from_scale(16.0, 16.0);

    Pixmap pixmap = new_pixmap(200, 200);
    stroke_path(pixmap, path, paint, stroke, transform, nullptr);

    save(pixmap, "stroke/round-caps-and-large-scale.png");
}

SCENE(stroke_circle) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    SkPath path = *path_from_circle(100.0, 100.0, 50.0);
    Stroke stroke;
    stroke.width = 2.0;

    Pixmap pixmap = new_pixmap(200, 200);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "stroke/circle.png");
}

SCENE(stroke_zero_len_subpath_butt_cap) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(50.0, 50.0);
    pb.line_to(50.0, 50.0);
    SkPath path = *pb.finish();

    Stroke stroke;
    stroke.width = 20.0;
    stroke.line_cap = LineCap::Butt;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "stroke/zero-len-subpath-butt-cap.png");
}

SCENE(stroke_zero_len_subpath_round_cap) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(50.0, 50.0);
    pb.line_to(50.0, 50.0);
    SkPath path = *pb.finish();

    Stroke stroke;
    stroke.width = 20.0;
    stroke.line_cap = LineCap::Round;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "stroke/zero-len-subpath-round-cap.png");
}

SCENE(stroke_zero_len_subpath_square_cap) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(50.0, 50.0);
    pb.line_to(50.0, 50.0);
    SkPath path = *pb.finish();

    Stroke stroke;
    stroke.width = 20.0;
    stroke.line_cap = LineCap::Square;

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "stroke/zero-len-subpath-square-cap.png");
}

SCENE(stroke_round_cap_join) {
    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    PathBuilder pb;
    pb.move_to(170.0, 30.0);
    pb.line_to(30.553378, 99.048418);
    pb.cubic_to(30.563658, 99.066835, 30.546308, 99.280724, 30.557592, 99.305282);
    SkPath path = *pb.finish();

    Stroke stroke;
    stroke.width = 30.0;
    stroke.line_cap = LineCap::Round;
    stroke.line_join = LineJoin::Round;

    Pixmap pixmap = new_pixmap(200, 200);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "stroke/round-cap-join.png");
}
