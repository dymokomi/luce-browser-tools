// scenes_gradients.cpp - tiny-skia tests/integration/gradients.rs, rendered with Skia m144.
//
// force_hq_pipeline has no Skia equivalent: the *_hq scenes render like the *_lq ones.
#include "oracle.h"

SCENE(gradients_two_stops_linear_pad_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-pad-lq.png");
}

SCENE(gradients_two_stops_linear_repeat_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {100.0, 100.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Repeat, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-repeat-lq.png");
}

SCENE(gradients_two_stops_linear_reflect_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {100.0, 100.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Reflect, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-reflect-lq.png");
}

SCENE(gradients_three_stops_evenly_spaced_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.25, rgba8(50, 127, 150, 200)}, {0.50, rgba8(220, 140, 75, 180)}, {0.75, rgba8(40, 180, 55, 160)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/three-stops-evenly-spaced-lq.png");
}

SCENE(gradients_two_stops_unevenly_spaced_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.25, rgba8(50, 127, 150, 200)}, {0.75, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-unevenly-spaced-lq.png");
}

SCENE(gradients_two_stops_linear_pad_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-pad-hq.png");
}

SCENE(gradients_two_stops_linear_repeat_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {100.0, 100.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Repeat, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-repeat-hq.png");
}

SCENE(gradients_two_stops_linear_reflect_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {100.0, 100.0}, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Reflect, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-linear-reflect-hq.png");
}

SCENE(gradients_three_stops_evenly_spaced_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.25, rgba8(50, 127, 150, 200)}, {0.50, rgba8(220, 140, 75, 180)}, {0.75, rgba8(40, 180, 55, 160)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/three-stops-evenly-spaced-hq.png");
}

SCENE(gradients_two_stops_unevenly_spaced_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = linear_gradient({10.0, 10.0}, {190.0, 190.0}, {{0.25, rgba8(50, 127, 150, 200)}, {0.75, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/two-stops-unevenly-spaced-hq.png");
}

SCENE(gradients_well_behaved_radial) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {120.0, 80.0}, 100.0, {{0.25, rgba8(50, 127, 150, 200)}, {0.75, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/well-behaved-radial.png");
}

SCENE(gradients_focal_on_circle_radial) {
    // 28.29: this radius forces the required pipeline stage.
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {120.0, 80.0}, 28.29, {{0.25, rgba8(50, 127, 150, 200)}, {0.75, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/focal-on-circle-radial.png");
}

SCENE(gradients_conical_greater_radial) {
    // 10.0: this radius forces the required pipeline stage.
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {120.0, 80.0}, 10.0, {{0.25, rgba8(50, 127, 150, 200)}, {0.75, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/conical-greater-radial.png");
}

SCENE(gradients_simple_radial_lq) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {100.0, 100.0}, 100.0, {{0.25, rgba8(50, 127, 150, 200)}, {1.00, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/simple-radial-lq.png");
}

SCENE(gradients_simple_radial_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {100.0, 100.0}, 100.0, {{0.25, rgba8(50, 127, 150, 200)}, {1.00, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/simple-radial-hq.png");
}

SCENE(gradients_simple_radial_with_ts_hq) {
    Paint paint;
    // paint.force_hq_pipeline = true: no Skia equivalent
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {100.0, 100.0}, 100.0, {{0.25, rgba8(50, 127, 150, 200)}, {1.00, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, from_row(2.0, 0.3, -0.7, 1.2, 10.5, -12.3));

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/simple-radial-with-ts-hq.png");
}

// Gradient doesn't add the Premultiply stage when all stops are opaque.
// But it checks colors only on creation, so we have to recheck them after calling `apply_opacity`.
SCENE(gradients_global_opacity) {
    Paint paint;
    paint.anti_alias = false;
    std::vector<GradientStop> stops = {
        {0.25, rgba8(50, 127, 150, 255)}, // no opacity here
        {1.00, rgba8(220, 140, 75, 255)}, // no opacity here
    };
    // paint.shader.apply_opacity(0.5): the Skia shader is immutable, so the stops get it first.
    apply_opacity(stops, 0.5);
    paint.shader = radial_gradient({100.0, 100.0}, 0.0, {100.0, 100.0}, 100.0, stops, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/global-opacity.png");
}

SCENE(gradients_strip_gradient) {
    // Equal radii, different centers creates a Strip gradient
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({50.0, 100.0}, 50.0, {150.0, 100.0}, 50.0, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/strip-gradient.png");
}

SCENE(gradients_concentric_radial) {
    // Same center, non-zero start radius (concentric gradient)
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 30.0, {100.0, 100.0}, 90.0, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/concentric-radial.png");
}

SCENE(gradients_conical_smaller_radial) {
    // Configuration that triggers XYTo2PtConicalSmaller stage
    Paint paint;
    paint.anti_alias = false;
    paint.shader = radial_gradient({100.0, 100.0}, 60.0, {150.0, 100.0}, 30.0, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/conical-smaller-radial.png");
}

SCENE(gradients_sweep_gradient) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = sweep_gradient({100.0, 100.0}, 135.0, 225.0, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/sweep-gradient.png");
}

SCENE(gradients_sweep_gradient_full) {
    Paint paint;
    paint.anti_alias = false;
    paint.shader = sweep_gradient({100.0, 100.0}, 0.0, 360.0, {{0.0, rgba8(50, 127, 150, 200)}, {1.0, rgba8(220, 140, 75, 180)}}, SpreadMode::Pad, identity());

    SkPath path = path_from_rect(rect_ltrb(10.0, 10.0, 190.0, 190.0));

    Pixmap pixmap = new_pixmap(200, 200);
    fill_path(pixmap, path, paint, FillRule::Winding, identity(), nullptr);

    save(pixmap, "gradients/sweep-gradient-full.png");
}
