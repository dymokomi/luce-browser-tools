// Oracle for display_list's box shadows: the pixels DisplayListPlayerSkia (Skia m144's raster
// blur mask filter: BlurRect nine-patches, rounded-rectangle nine-patches, SkMaskBlurFilter on
// whole masks, small and large sigmas) renders for outer and inner box shadow commands,
// printed as premultiplied 0xAARRGGBB words (gen_data.py makes tests_skia_reference_data_7.lucb).
#include <AK/Format.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ImmutableBitmap.h>
#include <LibGfx/PaintingSurface.h>
#include <LibWeb/Painting/AccumulatedVisualContext.h>
#include <LibWeb/Painting/DisplayList.h>
#include <LibWeb/Painting/DisplayListPlayerSkia.h>
#include <LibWeb/Painting/DisplayListRecorder.h>
#include <LibWeb/Painting/ScrollState.h>

using namespace Web::Painting;

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

static CornerRadii radii(int tl, int tr, int br, int bl)
{
    return { { tl, tl }, { tr, tr }, { br, br }, { bl, bl } };
}

static void outer(char const* name, int blur, Gfx::IntRect shadow, CornerRadii shadow_radii, Gfx::IntRect content, CornerRadii content_radii)
{
    scene(name, 40, 32, [&](auto& r) {
        r.paint_outer_box_shadow({ .color = Gfx::Color(200, 0, 120, 255), .blur_radius = blur, .device_content_rect = content, .content_corner_radii = content_radii, .shadow_rect = shadow, .shadow_corner_radii = shadow_radii });
    });
}

int main()
{
    // A rectangle: BlurRect's nine-patch (sigma 2 and 3), and a small one (a whole mask).
    outer("shadow_rect", 5, { 8, 6, 22, 18 }, {}, { 6, 4, 22, 18 }, {});
    outer("shadow_rect_sigma3", 7, { 9, 8, 20, 14 }, {}, { 7, 6, 20, 14 }, {});
    outer("shadow_rect_small", 9, { 14, 12, 8, 6 }, {}, { 12, 10, 8, 6 }, {});
    // Rounded rectangles: the nine-patch of a blurred small rounded rectangle, with the small
    // (sigma 1) and the large blur.
    outer("shadow_rrect", 6, { 6, 5, 28, 22 }, radii(6, 3, 8, 2), { 4, 3, 28, 22 }, radii(6, 3, 8, 2));
    outer("shadow_rrect_sigma1", 3, { 6, 5, 28, 22 }, radii(5, 5, 5, 5), { 4, 3, 28, 22 }, radii(5, 5, 5, 5));
    // An oval and a rounded rectangle too small for a nine-patch: whole blurred masks.
    outer("shadow_oval", 4, { 8, 6, 24, 18 }, radii(30, 30, 30, 30), { 6, 4, 24, 18 }, radii(30, 30, 30, 30));
    outer("shadow_rrect_small", 6, { 12, 10, 12, 10 }, radii(4, 4, 4, 4), { 10, 8, 12, 10 }, radii(4, 4, 4, 4));
    // An inner shadow of two rectangles: the nested rectangles' nine-patch.
    scene("shadow_inner", 40, 32, [](auto& r) {
        r.paint_inner_box_shadow({ .color = Gfx::Color(0, 60, 200, 255), .blur_radius = 6, .device_content_rect = { 4, 4, 32, 24 }, .content_corner_radii = {}, .outer_shadow_rect = { 0, 0, 40, 32 }, .inner_shadow_rect = { 9, 8, 24, 18 }, .inner_shadow_corner_radii = {} });
    });
    return 0;
}
