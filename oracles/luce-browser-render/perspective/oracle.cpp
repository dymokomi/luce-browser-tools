// Oracle for display_list's CPU player under 3D transforms: the pixels DisplayListPlayerSkia
// renders (Skia m144's raster SkCanvas with an SkM44 whose perspective row reaches every draw)
// for display lists whose accumulated visual contexts hold CSS perspective and 3D transform
// matrices, printed as premultiplied 0xAARRGGBB words, a row per line (gen_data.py turns them
// into luce-browser-render's tests_skia_reference_data_4.lucb).
#include <AK/Format.h>
#include <AK/Utf16String.h>
#include <LibCore/File.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/FontData.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/Typeface.h>
#include <LibGfx/ImmutableBitmap.h>
#include <LibGfx/Matrix4x4.h>
#include <LibGfx/PaintingSurface.h>
#include <LibGfx/TextLayout.h>
#include <LibWeb/Painting/AccumulatedVisualContext.h>
#include <LibWeb/Painting/DisplayList.h>
#include <LibWeb/Painting/DisplayListPlayerSkia.h>
#include <LibWeb/Painting/DisplayListRecorder.h>
#include <LibWeb/Painting/ScrollState.h>

using namespace Web::Painting;

struct FakeFontConfig final : Gfx::SystemFontProvider {
    StringView name() const override { return "FontConfig"sv; }
    RefPtr<Gfx::Font> get_font(FlyString const&, float, unsigned, unsigned, unsigned, Optional<Gfx::FontVariationSettings> const&, Optional<Gfx::ShapeFeatures> const&) override { return {}; }
    void for_each_typeface_with_family_name(FlyString const&, Function<void(Gfx::Typeface const&)>) override { }
};

static NonnullRefPtr<Gfx::Typeface> load(char const* name)
{
    auto path = ByteString::formatted("/Users/sedov/Dev/luce_dev/luce-browser-render-fid2/tests/web_fonts/fonts/{}", name);
    auto file = MUST(Core::File::open(path, Core::File::OpenMode::Read));
    auto bytes = MUST(file->read_until_eof());
    return MUST(Gfx::Typeface::try_load_from_font_data(Gfx::FontData::create_from_byte_buffer(move(bytes))));
}

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
    f(rec, *tree);
    auto surface = Gfx::PaintingSurface::create_with_size({ w, h }, Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied);
    DisplayListPlayerSkia player;
    ScrollStateSnapshot snap;
    player.execute(*dl, snap, surface);
    dump(name, *surface, w, h);
}

// The perspective matrix ViewportPaintable's compute_perspective_matrix builds for
// `perspective: d` with its origin at (ox, oy).
static Gfx::FloatMatrix4x4 perspective_about(float d, float ox, float oy)
{
    Gfx::FloatMatrix4x4 p(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -1 / d, 1);
    return Gfx::translation_matrix(Vector3<float>(ox, oy, 0)) * p * Gfx::translation_matrix(Vector3<float>(-ox, -oy, 0));
}

// CSS rotateX / rotateY with the given cosine and sine.
static Gfx::FloatMatrix4x4 rotate_x(float c, float s) { return Gfx::FloatMatrix4x4(1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1); }
static Gfx::FloatMatrix4x4 rotate_y(float c, float s) { return Gfx::FloatMatrix4x4(c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1); }

// A perspective node with a 3D transform node below it; the commands that follow draw in it.
static void enter_3d(DisplayListRecorder& r, AccumulatedVisualContextTree& tree, Gfx::FloatMatrix4x4 perspective, Gfx::FloatMatrix4x4 transform, Gfx::FloatPoint origin)
{
    auto p = tree.append(PerspectiveData { perspective }, {});
    auto t = tree.append(TransformData { transform, origin }, p);
    r.set_accumulated_visual_context(t);
}

static Gfx::Path star()
{
    Gfx::Path p;
    p.move_to({ 12, 1 });
    p.line_to({ 15, 21 });
    p.line_to({ 2, 8 });
    p.line_to({ 22, 8 });
    p.line_to({ 9, 21 });
    p.close();
    return p;
}

static Gfx::Path curves()
{
    Gfx::Path p;
    p.move_to({ 2, 12 });
    p.cubic_bezier_curve_to({ 2, 2 }, { 22, 2 }, { 22, 12 });
    p.quadratic_bezier_curve_to({ 12, 26 }, { 2, 12 });
    p.close();
    return p;
}

// The filter oracle's pattern: a gradient of colors with varying alpha, premultiplied.
static uint32_t pattern_pixel(int i, int j, int w, int h)
{
    int r = (i * 255) / (w - 1), g = (j * 255) / (h - 1), b = 255 - (i + j) * 13, a = 255 - ((i * 7 + j * 11) % 5) * 40;
    if (b < 0)
        b = 0;
    auto mul = [](int c, int a) { int prod = c * a + 128; return (prod + (prod >> 8)) >> 8; };
    return (uint32_t(a) << 24) | (uint32_t(mul(r, a)) << 16) | (uint32_t(mul(g, a)) << 8) | uint32_t(mul(b, a));
}

static NonnullRefPtr<Gfx::ImmutableBitmap> pattern_bitmap(int w, int h)
{
    auto bitmap = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied, { w, h }));
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            bitmap->scanline(j)[i] = pattern_pixel(i, j, w, h);
    return Gfx::ImmutableBitmap::create(bitmap);
}

int main()
{
    Gfx::FontDatabase::the().install_system_font_provider(make<FakeFontConfig>());
    auto serenity = load("SerenitySans-Regular.ttf");
    auto s16 = serenity->font(12); // 16 px

    scene("persp_rotate_x", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(64, 16, 16), rotate_x(0.70710677f, 0.70710677f), { 16, 16 });
        r.fill_rect({ 4, 4, 24, 24 }, Gfx::Color(0, 128, 255, 255));
    });
    scene("persp_rotate_y_curves", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(32, 16, 16), rotate_y(0.5f, 0.8660254f), { 12, 12 });
        r.fill_path({ .path = curves(), .paint_style_or_color = Gfx::Color(200, 30, 60, 255), .winding_rule = Gfx::WindingRule::Nonzero });
    });
    scene("persp_w_clip", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(8, 16, 16), rotate_y(0.25881904f, 0.9659258f), { 16, 16 });
        r.fill_rect({ 2, 4, 28, 24 }, Gfx::Color(0, 160, 80, 255));
    });
    scene("persp_stroke", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(48, 16, 16), rotate_x(0.81915206f, -0.57357645f), { 16, 16 });
        r.stroke_path({ .cap_style = Gfx::Path::CapStyle::Butt, .join_style = Gfx::Path::JoinStyle::Miter, .miter_limit = 4, .dash_array = {}, .dash_offset = 0, .path = star(), .paint_style_or_color = Gfx::Color(0, 0, 0, 255), .thickness = 2.0f });
    });
    scene("persp_clip_rect", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(48, 16, 16), rotate_y(0.76604444f, 0.64278764f), { 16, 16 });
        r.save();
        r.add_clip_rect({ 6, 6, 16, 16 });
        r.fill_rect({ 0, 0, 32, 32 }, Gfx::Color(255, 100, 0, 255));
        r.restore();
    });
    scene("persp_opacity", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(64, 16, 16), rotate_x(0.70710677f, 0.70710677f), { 16, 16 });
        r.apply_effects(0.5f);
        r.fill_rect({ 4, 4, 24, 24 }, Gfx::Color(0, 0, 255, 255));
        r.restore();
    });
    scene("persp_translate_z", 32, 32, [](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(16, 0, 16), Gfx::FloatMatrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 4, 0, 0, 0, 1), { 8, 8 });
        r.fill_rect({ 4, 8, 12, 12 }, Gfx::Color(0, 128, 0, 255));
    });
    auto image = pattern_bitmap(6, 5);
    scene("persp_image_nearest", 32, 32, [&](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(48, 16, 16), rotate_y(0.8660254f, 0.5f), { 16, 16 });
        r.draw_scaled_immutable_bitmap({ 4, 4, 24, 24 }, { 0, 0, 32, 32 }, *image, Gfx::ScalingMode::NearestNeighbor);
    });
    scene("persp_image_bilinear", 32, 32, [&](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(48, 16, 16), rotate_y(0.8660254f, 0.5f), { 16, 16 });
        r.draw_scaled_immutable_bitmap({ 4, 4, 24, 24 }, { 0, 0, 32, 32 }, *image, Gfx::ScalingMode::Bilinear);
    });
    scene("persp_text", 40, 24, [&](auto& r, auto& tree) {
        enter_3d(r, tree, perspective_about(64, 20, 12), rotate_x(0.8660254f, 0.5f), { 20, 12 });
        auto text = Utf16String::from_utf8("Hi"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s16, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 4, 16 }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 40, 24 }, 1.0, Gfx::Orientation::Horizontal);
    });
    return 0;
}
