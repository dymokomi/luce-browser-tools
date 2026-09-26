// Oracle for display_list's CPU player scenes: pixels DisplayListPlayerSkia renders (donor build).
#include <AK/Format.h>
#include <AK/Utf16String.h>
#include <LibCore/File.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Filter.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/FontData.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/Typeface.h>
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
    auto path = ByteString::formatted("/Users/sedov/Dev/luce_dev/luce-browser-render-integrate/tests/web_fonts/fonts/{}", name);
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

int main()
{
    Gfx::FontDatabase::the().install_system_font_provider(make<FakeFontConfig>());
    auto serenity = load("SerenitySans-Regular.ttf");
    auto lato = load("Lato-Bold.ttf");
    auto s16 = serenity->font(12); // 16 px
    auto l20 = lato->font(15);     // 20 px

    scene("fill_path_even_odd", 24, 24, [](auto& r, auto&) { r.fill_path({ .path = star(), .paint_style_or_color = Gfx::Color(0, 0, 255, 255), .winding_rule = Gfx::WindingRule::EvenOdd }); });
    scene("fill_path_nonzero", 24, 24, [](auto& r, auto&) { r.fill_path({ .path = star(), .paint_style_or_color = Gfx::Color(0, 128, 0, 255), .winding_rule = Gfx::WindingRule::Nonzero }); });
    scene("fill_path_curves", 24, 24, [](auto& r, auto&) { r.fill_path({ .path = curves(), .opacity = 0.75f, .paint_style_or_color = Gfx::Color(200, 30, 60, 255), .winding_rule = Gfx::WindingRule::Nonzero }); });
    scene("stroke_path", 24, 24, [](auto& r, auto&) {
        r.stroke_path({ .cap_style = Gfx::Path::CapStyle::Round, .join_style = Gfx::Path::JoinStyle::Round, .miter_limit = 4, .dash_array = {}, .dash_offset = 0, .path = curves(), .paint_style_or_color = Gfx::Color(0, 0, 0, 255), .thickness = 2.5f });
    });
    scene("layer_in_rounded_clip", 16, 16, [](auto& r, auto&) {
        r.fill_rect({ 0, 0, 16, 16 }, Gfx::Color(0, 0, 255, 255));
        r.save();
        r.add_rounded_rect_clip({ { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 } }, { 2, 2, 12, 12 }, CornerClip::Outside);
        r.apply_effects(0.5f);
        r.fill_rect({ 0, 0, 16, 16 }, Gfx::Color(255, 0, 0, 255));
        r.restore();
        r.restore();
    });
    scene("glyph_run", 40, 20, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("Hi, fox"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s16, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 2, 15 }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 40, 20 }, 1.0, Gfx::Orientation::Horizontal);
    });
    scene("glyph_run_scaled", 48, 32, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("AVo"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *l20, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 1.5f, 25.25f }, *run, Gfx::Color(20, 90, 200, 255), { 0, 0, 48, 32 }, 1.5, Gfx::Orientation::Horizontal);
    });
    scene("text_shadow", 40, 24, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("Hi"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *l20, Gfx::GlyphRun::TextType::Common);
        run->ensure_text_blob(1.0);
        r.paint_text_shadow(4, { 0, 0, 40, 24 }, { 4, 2, 30, 20 }, *run, 1.0, Gfx::Color(255, 0, 0, 255), { 1, 14 });
    });
    scene("filter_blur", 24, 24, [](auto& r, auto&) {
        r.apply_effects(1.0f, Gfx::CompositingAndBlendingOperator::Normal, Gfx::Filter::blur(2.0f, 2.0f));
        r.fill_rect({ 6, 6, 12, 12 }, Gfx::Color(0, 0, 255, 255));
        r.restore();
    });
    scene("filter_drop_shadow_opacity", 24, 24, [](auto& r, auto&) {
        r.apply_effects(0.8f, Gfx::CompositingAndBlendingOperator::Normal, Gfx::Filter::drop_shadow(2.0f, 3.0f, 1.5f, Gfx::Color(0, 0, 0, 160)));
        r.fill_rect({ 4, 4, 12, 10 }, Gfx::Color(255, 128, 0, 255));
        r.restore();
    });
    scene("filter_color_chain", 16, 16, [](auto& r, auto&) {
        auto grayscale = Gfx::Filter::color(Gfx::ColorFilterType::Grayscale, 0.5f);
        auto hue = Gfx::Filter::hue_rotate(90.0f, grayscale);
        r.apply_effects(1.0f, Gfx::CompositingAndBlendingOperator::Multiply, hue);
        r.fill_rect({ 2, 2, 12, 12 }, Gfx::Color(200, 40, 90, 200));
        r.restore();
    });
    scene("filter_blur_clipped_scaled", 24, 24, [](auto& r, auto& tree) {
        auto m = Gfx::scale_matrix<float>({ 1.5f, 1.5f, 1 });
        auto index = tree.append(TransformData { m, { 0, 0 } }, {});
        r.set_accumulated_visual_context(index);
        r.save();
        r.add_clip_rect({ 2, 2, 12, 12 });
        r.apply_effects(1.0f, Gfx::CompositingAndBlendingOperator::Normal, Gfx::Filter::blur(1.0f, 1.0f));
        r.fill_rect({ 4, 4, 6, 6 }, Gfx::Color(0, 160, 0, 255));
        r.restore();
        r.restore();
    });
    scene("backdrop_blur", 24, 24, [](auto& r, auto&) {
        r.fill_rect({ 0, 0, 12, 24 }, Gfx::Color(255, 0, 0, 255));
        r.fill_rect({ 12, 0, 12, 24 }, Gfx::Color(0, 0, 255, 255));
        r.apply_backdrop_filter({ 4, 4, 16, 16 }, {}, Gfx::Filter::blur(2.0f, 2.0f));
    });
    return 0;
}
