// Oracle for display_list's images: the pixels DisplayListPlayerSkia (Skia m144's SkImageShader
// with to_skia_sampling_options' sampling: nearest, bilinear, bilinear with linear mipmaps)
// renders for scaled and repeated image commands, printed as premultiplied 0xAARRGGBB words
// (gen_data.py makes tests_skia_reference_data_6.lucb).
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
    auto odd = pattern_bitmap(37, 29);
    auto even = pattern_bitmap(64, 48);
    auto scaled = [](char const* name, Gfx::ImmutableBitmap const& image, Gfx::IntRect dst, Gfx::ScalingMode mode) {
        scene(name, 40, 32, [&](auto& r) { r.draw_scaled_immutable_bitmap(dst, { 0, 0, 40, 32 }, image, mode); });
    };
    scaled("image_mip_level_0", *odd, { 3, 2, 30, 24 }, Gfx::ScalingMode::BilinearMipmap);
    scaled("image_mip_fraction", *odd, { 2, 3, 15, 11 }, Gfx::ScalingMode::BilinearMipmap);
    scaled("image_mip_deep", *even, { 1, 1, 9, 7 }, Gfx::ScalingMode::BilinearMipmap);
    scaled("image_mip_aniso", *even, { 0, 0, 30, 10 }, Gfx::ScalingMode::BilinearMipmap);
    scaled("image_bilinear_down", *odd, { 2, 3, 15, 11 }, Gfx::ScalingMode::Bilinear);
    scaled("image_nearest_down", *odd, { 2, 3, 15, 11 }, Gfx::ScalingMode::NearestNeighbor);
    scene("image_mip_repeated", 40, 32, [&](auto& r) {
        r.draw_repeated_immutable_bitmap({ 3, 2, 13, 10 }, { 0, 0, 40, 32 }, odd, Gfx::ScalingMode::BilinearMipmap, true, true);
    });
    return 0;
}
