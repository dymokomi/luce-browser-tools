// scenes_skia_dash.cpp - tiny-skia tests/integration/skia_dash.rs, rendered with Skia m144.
//
// crbug_140642 and crbug_124652 only assert StrokeDash::new(..).is_some(): no scene.
#include "oracle.h"

// Extremely large path_length/dash_length ratios may cause infinite looping
// due to single precision rounding. No expected image: saved under extra/.
SCENE(skia_dash_infinite_dash) {
    PathBuilder pb;
    pb.move_to(0.0, 5.0);
    pb.line_to(5000000.0, 5.0);
    SkPath path = *pb.finish();

    Paint paint;
    paint.set_color_rgba8(50, 127, 150, 200);
    paint.anti_alias = true;

    Stroke stroke;
    stroke.dash = stroke_dash({0.2, 0.2}, 0.0);

    Pixmap pixmap = new_pixmap(100, 100);
    stroke_path(pixmap, path, paint, stroke, identity(), nullptr); // Doesn't draw anything.

    save(pixmap, "extra/skia-dash-infinite-dash.png");
}
