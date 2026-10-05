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
    auto path = ByteString::formatted("/Users/sedov/Dev/luce_dev/luce-browser-render-fid1/tests/web_fonts/fonts/{}", name);
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


// SkTMaskGamma_build_correcting_lut and the sRGB luminance, copied from Skia m144's
// src/core/SkMaskGamma.cpp (the library does not export them), for the tables.
static float toLuma(float luminance) { if (luminance <= 0.04045f) return luminance / 12.92f; return powf((luminance + 0.055f) / 1.055f, 2.4f); }
static float fromLuma(float luma) { if (luma <= 0.0031308f) return luma * 12.92f; return 1.055f * powf(luma, 1.0f / 2.4f) - 0.055f; }
static float apply_contrast(float srca, float contrast) { return srca + ((1.0f - srca) * contrast * srca); }
static void build_lut(uint8_t* table, unsigned srcI, float contrast)
{
    float const src = (float)srcI / 255.0f;
    float const linSrc = toLuma(src);
    float const dst = 1.0f - src;
    float const linDst = toLuma(dst);
    float const adjustedContrast = contrast * linDst;
    if (fabs(src - dst) < (1.0f / 256.0f)) {
        float ii = 0.0f;
        for (int i = 0; i < 256; ++i, ii += 1.0f) {
            float rawSrca = ii / 255.0f;
            float srca = apply_contrast(rawSrca, adjustedContrast);
            table[i] = (uint8_t)(int)floor((double)(255.0f * srca) + 0.5);
        }
    } else {
        float ii = 0.0f;
        for (int i = 0; i < 256; ++i, ii += 1.0f) {
            float rawSrca = ii / 255.0f;
            float srca = apply_contrast(rawSrca, adjustedContrast);
            float dsta = 1.0f - srca;
            float linOut = (linSrc * srca + dsta * linDst);
            float out = fromLuma(linOut);
            float result = (out - dst) / (src - dst);
            table[i] = (uint8_t)(int)floor((double)(255.0f * result) + 0.5);
        }
    }
}

int main()
{
    Gfx::FontDatabase::the().install_system_font_provider(make<FakeFontConfig>());
    auto serenity = load("SerenitySans-Regular.ttf");
    auto s16 = serenity->font(12); // 16 px
    auto s12 = serenity->font(9);  // 12 px

    {
        outln("scene mask_gamma");
        for (unsigned i = 0; i < 8; ++i) {
            unsigned base = i << 5;
            unsigned lum = base | (base >> 3) | (base >> 6);
            uint8_t table[256];
            build_lut(table, lum, 128.0f / 255.0f);
            StringBuilder sb;
            for (int k = 0; k < 256; ++k)
                sb.appendff("{:08x} ", table[k]);
            outln("{}", sb.string_view());
        }
    }
    scene("glyph_run_phases", 48, 40, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("fox"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s16, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 1.1f, 12.4f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 24.3f, 12.6f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 1.55f, 30.5f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 24.8f, 31.45f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.0, Gfx::Orientation::Horizontal);
    });
    scene("glyph_run_colors", 48, 24, [&](auto& r, auto&) {
        r.fill_rect({ 0, 0, 24, 24 }, Gfx::Color(255, 165, 0, 255));
        r.fill_rect({ 24, 0, 24, 24 }, Gfx::Color(255, 0, 255, 255));
        auto text = Utf16String::from_utf8("ab"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s16, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 2.25f, 4 }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 24 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 26.5f, 4 }, *run, Gfx::Color(255, 255, 255, 255), { 0, 0, 48, 24 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 2.75f, 14 }, *run, Gfx::Color(30, 140, 60, 255), { 0, 0, 48, 24 }, 1.0, Gfx::Orientation::Horizontal);
        r.draw_glyph_run({ 26, 14 }, *run, Gfx::Color(200, 200, 40, 128), { 0, 0, 48, 24 }, 1.0, Gfx::Orientation::Horizontal);
    });
    scene("glyph_run_vertical", 24, 40, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("Hey"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s12, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 2.3f, 4.6f }, *run, Gfx::Color(0, 0, 128, 255), { 3, 2, 18, 36 }, 1.0, Gfx::Orientation::Vertical);
    });
    scene("glyph_run_serenity_scaled", 64, 32, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("Hi, fox"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s12, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 1.5f, 20.25f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 64, 32 }, 1.5, Gfx::Orientation::Horizontal);
    });
    scene("glyph_run_notdef", 48, 40, [&](auto& r, auto&) {
        auto text = Utf16String::from_utf8("\u05d0\u05d1a"sv);
        auto run = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s16, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 1.3f, 12.6f }, *run, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.0, Gfx::Orientation::Horizontal);
        auto run12 = Gfx::shape_text({ 0, 0 }, 0, text.utf16_view(), *s12, Gfx::GlyphRun::TextType::Common);
        r.draw_glyph_run({ 2.7f, 31.4f }, *run12, Gfx::Color(0, 0, 0, 255), { 0, 0, 48, 40 }, 1.5, Gfx::Orientation::Horizontal);
    });
    return 0;
}
