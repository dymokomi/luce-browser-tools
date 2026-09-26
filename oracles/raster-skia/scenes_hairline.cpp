// scenes_hairline.cpp - tiny-skia tests/integration/hairline.rs, rendered with Skia m144.
#include "oracle.h"

static Pixmap draw_line(float x0, float y0, float x1, float y1, bool anti_alias, float width, LineCap line_cap) {
    Pixmap pixmap = new_pixmap(100, 100);

    PathBuilder pb;
    pb.move_to(x0, y0);
    pb.line_to(x1, y1);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = anti_alias;

    Stroke stroke;
    stroke.width = width;
    stroke.line_cap = line_cap;
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    return pixmap;
}

SCENE(hairline_hline_05) { save(draw_line(10.0, 10.0, 90.0, 10.0, false, 0.5, LineCap::Butt), "hairline/hline-05.png"); }
SCENE(hairline_hline_05_aa) { save(draw_line(10.0, 10.0, 90.0, 10.0, true, 0.5, LineCap::Butt), "hairline/hline-05-aa.png"); }
SCENE(hairline_hline_05_aa_round) { save(draw_line(10.0, 10.0, 90.0, 10.0, true, 0.5, LineCap::Round), "hairline/hline-05-aa-round.png"); }
SCENE(hairline_vline_05) { save(draw_line(10.0, 10.0, 10.0, 90.0, false, 0.5, LineCap::Butt), "hairline/vline-05.png"); }
SCENE(hairline_vline_05_aa) { save(draw_line(10.0, 10.0, 10.0, 90.0, true, 0.5, LineCap::Butt), "hairline/vline-05-aa.png"); }
SCENE(hairline_vline_05_aa_round) { save(draw_line(10.0, 10.0, 10.0, 90.0, true, 0.5, LineCap::Round), "hairline/vline-05-aa-round.png"); }
SCENE(hairline_horish_05_aa) { save(draw_line(10.0, 10.0, 90.0, 70.0, true, 0.5, LineCap::Butt), "hairline/horish-05-aa.png"); }
SCENE(hairline_vertish_05_aa) { save(draw_line(10.0, 10.0, 70.0, 90.0, true, 0.5, LineCap::Butt), "hairline/vertish-05-aa.png"); }
SCENE(hairline_clip_line_05_aa) { save(draw_line(-10.0, 10.0, 110.0, 70.0, true, 0.5, LineCap::Butt), "hairline/clip-line-05-aa.png"); }
SCENE(hairline_clip_line_00) { save(draw_line(-10.0, 10.0, 110.0, 70.0, false, 0.0, LineCap::Butt), "hairline/clip-line-00.png"); }

SCENE(hairline_clip_line_00_v2) {
    Pixmap pixmap = new_pixmap(512, 512);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = false;

    Stroke stroke;
    stroke.width = 0.0;

    PathBuilder builder;
    builder.move_to(369.26462, 577.8069);
    builder.line_to(488.0846, 471.04388);
    SkPath path = *builder.finish();
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "hairline/clip-line-00-v2.png");
}

SCENE(hairline_clip_hline_top_aa) { save(draw_line(-1.0, 0.0, 101.0, 0.0, true, 1.0, LineCap::Butt), "hairline/clip-hline-top-aa.png"); }
SCENE(hairline_clip_hline_bottom_aa) { save(draw_line(-1.0, 100.0, 101.0, 100.0, true, 1.0, LineCap::Butt), "hairline/clip-hline-bottom-aa.png"); }
SCENE(hairline_clip_vline_left_aa) { save(draw_line(0.0, -1.0, 0.0, 101.0, true, 1.0, LineCap::Butt), "hairline/clip-vline-left-aa.png"); }
SCENE(hairline_clip_vline_right_aa) { save(draw_line(100.0, -1.0, 100.0, 101.0, true, 1.0, LineCap::Butt), "hairline/clip-vline-right-aa.png"); }

static Pixmap draw_quad(bool anti_alias, float width, LineCap line_cap) {
    Pixmap pixmap = new_pixmap(200, 100);

    PathBuilder pb;
    pb.move_to(25.0, 80.0);
    pb.quad_to(155.0, 75.0, 175.0, 20.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = anti_alias;

    Stroke stroke;
    stroke.width = width;
    stroke.line_cap = line_cap;
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    return pixmap;
}

SCENE(hairline_quad_width_05_aa) { save(draw_quad(true, 0.5, LineCap::Butt), "hairline/quad-width-05-aa.png"); }
SCENE(hairline_quad_width_05_aa_round) { save(draw_quad(true, 0.5, LineCap::Round), "hairline/quad-width-05-aa-round.png"); }
SCENE(hairline_quad_width_00) { save(draw_quad(false, 0.0, LineCap::Butt), "hairline/quad-width-00.png"); }

static Pixmap draw_cubic(const float (&points)[8], bool anti_alias, float width, LineCap line_cap) {
    Pixmap pixmap = new_pixmap(200, 100);

    PathBuilder pb;
    pb.move_to(points[0], points[1]);
    pb.cubic_to(points[2], points[3], points[4], points[5], points[6], points[7]);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = anti_alias;

    Stroke stroke;
    stroke.width = width;
    stroke.line_cap = line_cap;
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    return pixmap;
}

SCENE(hairline_cubic_width_10_aa) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, true, 1.0, LineCap::Butt), "hairline/cubic-width-10-aa.png"); }
SCENE(hairline_cubic_width_05_aa) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, true, 0.5, LineCap::Butt), "hairline/cubic-width-05-aa.png"); }
SCENE(hairline_cubic_width_00_aa) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, true, 0.0, LineCap::Butt), "hairline/cubic-width-00-aa.png"); }
SCENE(hairline_cubic_width_00) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, false, 0.0, LineCap::Butt), "hairline/cubic-width-00.png"); }
SCENE(hairline_cubic_width_05_aa_round) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, true, 0.5, LineCap::Round), "hairline/cubic-width-05-aa-round.png"); }
SCENE(hairline_cubic_width_00_round) { save(draw_cubic({25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, false, 0.0, LineCap::Round), "hairline/cubic-width-00-round.png"); }
// This curve will invoke `path_geometry::chop_cubic_at_max_curvature` branch of `hair_cubic`.
SCENE(hairline_chop_cubic_01) { save(draw_cubic({57.0, 13.0, 17.0, 15.0, 55.0, 97.0, 89.0, 62.0}, true, 0.5, LineCap::Butt), "hairline/chop-cubic-01.png"); }
SCENE(hairline_clip_cubic_05_aa) { save(draw_cubic({-25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, true, 0.5, LineCap::Butt), "hairline/clip-cubic-05-aa.png"); }
SCENE(hairline_clip_cubic_00) { save(draw_cubic({-25.0, 80.0, 55.0, 25.0, 155.0, 75.0, 175.0, 20.0}, false, 0.0, LineCap::Butt), "hairline/clip-cubic-00.png"); }

SCENE(hairline_clipped_circle_aa) {
    Pixmap pixmap = new_pixmap(100, 100);

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.width = 0.5;

    SkPath path = *path_from_circle(50.0, 50.0, 55.0);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr);

    save(pixmap, "hairline/clipped-circle-aa.png");
}
