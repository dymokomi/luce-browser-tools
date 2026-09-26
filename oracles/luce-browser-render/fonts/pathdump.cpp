#include <LibCore/MappedFile.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Font/PathFontProvider.h>
#include <LibGfx/Font/Typeface.h>
#include <core/SkFont.h>
#include <core/SkPath.h>
#include <stdio.h>
int main(int argc, char** argv) {
    auto& provider = static_cast<Gfx::PathFontProvider&>(Gfx::FontDatabase::the().install_system_font_provider(make<Gfx::PathFontProvider>()));
    provider.set_name_but_fixme_should_create_custom_system_font_provider("FontConfig"_string);
    auto file = MUST(Core::MappedFile::map({ argv[1], strlen(argv[1]) }));
    auto tf = MUST(Gfx::Typeface::try_load_from_temporary_memory(file->bytes()));
    auto font = tf->font(12);
    SkPath path; font->skia_font(1).getPath(tf->glyph_id_for_code_point(strtoul(argv[2], 0, 16)), &path);
    SkPath::RawIter it(path); SkPoint pts[4]; SkPath::Verb v;
    while ((v = it.next(pts)) != SkPath::kDone_Verb) { int np = v == 0 ? 1 : v == 1 ? 2 : v == 2 ? 3 : v == 4 ? 4 : 0; int st = v == 0 ? 0 : 1; printf("%d", v); for (int i = st; i < np; ++i) { unsigned a, b; memcpy(&a, &pts[i].fX, 4); memcpy(&b, &pts[i].fY, 4); printf(" %u,%u", a, b); }; printf("\n"); }
}
