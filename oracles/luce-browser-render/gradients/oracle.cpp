// Oracle for display_list's gradients: the pixels DisplayListPlayerSkia (Skia m144's
// SkGradientShader with Ladybird's Interpolation: premultiplied, in each CSS color space and
// hue method; dithered) renders for linear, radial and conic gradient commands, printed as
// premultiplied 0xAARRGGBB words (gen_data.py makes tests_skia_reference_data_5.lucb).
#include <AK/Format.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/PaintingSurface.h>
#include <LibWeb/Painting/AccumulatedVisualContext.h>
#include <LibWeb/Painting/DisplayList.h>
#include <LibWeb/Painting/DisplayListPlayerSkia.h>
#include <LibWeb/Painting/DisplayListRecorder.h>
#include <LibWeb/Painting/ScrollState.h>

using namespace Web::Painting;
using Method = Web::CSS::ColorInterpolationMethodStyleValue::ColorInterpolationMethod;
using Polar = Web::CSS::ColorInterpolationMethodStyleValue::PolarColorInterpolationMethod;
using Web::CSS::HueInterpolationMethod;
using Web::CSS::PolarColorSpace;
using Web::CSS::RectangularColorSpace;

static void dump(char const* name, Gfx::PaintingSurface& surface, int w, int h)
{
    auto bitmap = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied, { w, h }));
    surface.read_into_bitmap(*bitmap);
    outln("scene {}", name);
    for (int y = 0; y < h; ++y) {
        StringBuilder sb;
        for (int x = 0; x < w; ++x)
            sb.appendff("{:08x} ", bitmap->scanline(y)[x]);
        outln("{}", sb.string_view());
    }
}

template<typename F>
static void scene(char const* name, int w, int h, F f)
{
    auto tree = AccumulatedVisualContextTree::create();
    auto dl = DisplayList::create(tree);
    DisplayListRecorder rec(*dl);
    f(rec);
    auto surface = Gfx::PaintingSurface::create_with_size({ w, h }, Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied);
    DisplayListPlayerSkia player;
    ScrollStateSnapshot snap;
    player.execute(*dl, snap, surface);
    dump(name, *surface, w, h);
}

// Red, half-transparent green, blue, gray: hues that differ, an alpha to unpremultiply, and a
// powerless hue.
static ColorStopData stops()
{
    ColorStopData data;
    data.list.append({ .color = Gfx::Color(230, 20, 40, 255), .position = 0.0f });
    data.list.append({ .color = Gfx::Color(30, 200, 60, 128), .position = 0.35f });
    data.list.append({ .color = Gfx::Color(20, 40, 220, 255), .position = 0.7f });
    data.list.append({ .color = Gfx::Color(128, 128, 128, 255), .position = 1.0f });
    return data;
}

static void linear(char const* name, Method method, float angle = 63.0f)
{
    scene(name, 24, 16, [&](auto& r) {
        r.fill_rect_with_linear_gradient({ 0, 0, 24, 16 }, LinearGradientData { angle, stops(), method });
    });
}

int main()
{
    linear("grad_srgb", RectangularColorSpace::Srgb);
    linear("grad_srgb_linear", RectangularColorSpace::SrgbLinear);
    linear("grad_lab", RectangularColorSpace::Lab);
    linear("grad_oklab", RectangularColorSpace::Oklab);
    linear("grad_display_p3", RectangularColorSpace::DisplayP3);
    linear("grad_rec2020", RectangularColorSpace::Rec2020);
    linear("grad_prophoto", RectangularColorSpace::ProphotoRgb);
    linear("grad_a98", RectangularColorSpace::A98Rgb);
    linear("grad_hsl", Polar { PolarColorSpace::Hsl, HueInterpolationMethod::Shorter });
    linear("grad_hwb_longer", Polar { PolarColorSpace::Hwb, HueInterpolationMethod::Longer });
    linear("grad_lch_increasing", Polar { PolarColorSpace::Lch, HueInterpolationMethod::Increasing });
    linear("grad_oklch_decreasing", Polar { PolarColorSpace::Oklch, HueInterpolationMethod::Decreasing });
    linear("grad_oklch_angle", Polar { PolarColorSpace::Oklch, HueInterpolationMethod::Shorter }, -216.8699f);
    scene("grad_repeating", 24, 16, [](auto& r) {
        auto data = stops();
        data.repeat_length = 0.3f;
        data.list[0].position = 0.1f;
        data.list[1].position = 0.2f;
        data.list[2].position = 0.3f;
        data.list[3].position = 0.4f;
        r.fill_rect_with_linear_gradient({ 0, 0, 24, 16 }, LinearGradientData { 30.0f, data, RectangularColorSpace::Srgb });
    });
    scene("grad_radial", 24, 16, [](auto& r) {
        r.fill_rect_with_radial_gradient({ 0, 0, 24, 16 }, RadialGradientData { stops(), RectangularColorSpace::Oklab }, { 10, 7 }, { 14, 6 });
    });
    scene("grad_radial_degenerate", 8, 8, [](auto& r) {
        r.fill_rect_with_radial_gradient({ 0, 0, 8, 8 }, RadialGradientData { stops(), RectangularColorSpace::Srgb }, { 0, 0 }, { 0, 0 });
    });
    scene("grad_conic", 24, 16, [](auto& r) {
        r.fill_rect_with_conic_gradient({ 0, 0, 24, 16 }, ConicGradientData { 30.0f, stops(), Polar { PolarColorSpace::Hsl, HueInterpolationMethod::Shorter } }, { 11, 8 });
    });
    return 0;
}
