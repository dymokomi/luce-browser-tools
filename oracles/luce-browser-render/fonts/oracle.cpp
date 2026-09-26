// Local font oracle for region r09: prints what LibGfx (Skia + HarfBuzz) answers for a font file.
#include <AK/Utf16String.h>
#include <AK/Format.h>
#include <LibCore/MappedFile.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/Typeface.h>
#include <LibGfx/Font/TypefaceSkia.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/PathFontProvider.h>
#include <LibGfx/TextLayout.h>
#include <core/SkFont.h>
#include <core/SkFontMetrics.h>
#include <core/SkFontTypes.h>
#include <core/SkPath.h>
#include <core/SkFontMgr.h>
#include <core/SkData.h>
#include <core/SkStream.h>
#include <ports/SkFontMgr_empty.h>
#include <stdio.h>
#include <string.h>

static void print_ft(char const* path, float px, char const* text)
{
    auto mgr = SkFontMgr_New_Custom_Empty();
    auto data = SkData::MakeFromFileName(path);
    auto tf = mgr->makeFromData(data);
    if (!tf) { printf("ft: no typeface\n"); return; }
    SkFont font { tf, px };
    SkFontMetrics m;
    font.getMetrics(&m);
    printf("ft px=%g ascent=%.9g descent=%.9g leading=%.9g xheight=%.9g zero=%.9g\n", px, -m.fAscent, m.fDescent, m.fLeading, m.fXHeight, font.measureText("0", 1, SkTextEncoding::kUTF8));
    (void)text;
}

int main(int argc, char** argv)
{
    if (argc < 4) { fprintf(stderr, "usage: oracle FONT PX TEXT [glyph]\n"); return 1; }
    auto& provider = static_cast<Gfx::PathFontProvider&>(Gfx::FontDatabase::the().install_system_font_provider(make<Gfx::PathFontProvider>()));
    provider.set_name_but_fixme_should_create_custom_system_font_provider("FontConfig"_string);
    auto file = MUST(Core::MappedFile::map({ argv[1], strlen(argv[1]) }));
    auto typeface = MUST(Gfx::Typeface::try_load_from_temporary_memory(file->bytes()));
    float px = atof(argv[2]);
    float pt = px * 0.75f;
    auto font = typeface->font(pt);
    auto const& m = font->pixel_metrics();
    outln("family={} weight={} width={} slope={} upem={} glyphs={}", typeface->family(), typeface->weight(), typeface->width(), typeface->slope(), typeface->units_per_em(), typeface->glyph_count());
    printf("pixel_size=%.9g size=%.9g ascent=%.9g descent=%.9g line_gap=%.9g x_height=%.9g zero=%.9g\n", font->pixel_size(), m.size, m.ascent, m.descent, m.line_gap, m.x_height, m.advance_of_ascii_zero);
    auto sm = font->metrics();
    printf("scaled ascender=%.9g descender=%.9g line_gap=%.9g x_height=%.9g preferred_line_height=%.9g\n", sm.ascender, sm.descender, sm.line_gap, sm.x_height, font->preferred_line_height());
    printf("is_emoji=%d is_emoji=%d\n", font->is_emoji_font(), font->is_emoji_font());
    auto text = Utf16String::from_utf8(StringView { argv[3], strlen(argv[3]) });
    printf("width=%.9g\n", Gfx::measure_text_width(text.utf16_view(), *font));
    auto run = Gfx::shape_text({ 10, 20 }, 0, text.utf16_view(), *font, Gfx::GlyphRun::TextType::Common);
    printf("run width=%.9g\n", run->width());
    for (auto const& g : run->glyphs())
        printf("  glyph id=%u x=%.9g y=%.9g w=%.9g len=%zu\n", g.glyph_id, g.position.x(), g.position.y(), g.glyph_width, g.length_in_code_units);
    for (int i = 4; i < argc; ++i) {
        u16 gid = atoi(argv[i]);
        auto skfont = font->skia_font(1);
        SkPath path;
        if (skfont.getPath(gid, &path)) {
            SkPath::Iter it(path, false);
            SkPoint pts[4];
            SkPath::Verb v;
            printf("path %u:\n", gid);
            while ((v = it.next(pts)) != SkPath::kDone_Verb) {
                switch (v) {
                case SkPath::kMove_Verb: printf("  M %.9g %.9g\n", pts[0].fX, pts[0].fY); break;
                case SkPath::kLine_Verb: printf("  L %.9g %.9g\n", pts[1].fX, pts[1].fY); break;
                case SkPath::kQuad_Verb: printf("  Q %.9g %.9g %.9g %.9g\n", pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY); break;
                case SkPath::kCubic_Verb: printf("  C %.9g %.9g %.9g %.9g %.9g %.9g\n", pts[1].fX, pts[1].fY, pts[2].fX, pts[2].fY, pts[3].fX, pts[3].fY); break;
                case SkPath::kClose_Verb: printf("  Z\n"); break;
                default: break;
                }
            }
        }
    }
    print_ft(argv[1], px, argv[3]);
    return 0;
}
