// Oracle for gfx's PainterRaster::draw_bitmap of an image in another color space: the donor's
// PainterSkia (drawImageRect of the bitmap's SkImage, tagged with its Gfx::ColorSpace) into a
// bitmap-wrapping sRGB PaintingSurface, printed as the target's Gfx::Color values (0xAARRGGBB,
// premultiplied), for luce-browser-render's tests_gfx_paint_color.lucb.
#include <AK/Format.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ColorSpace.h>
#include <LibGfx/ImmutableBitmap.h>
#include <LibGfx/Painter.h>
#include <LibMedia/Color/CodingIndependentCodePoints.h>
#include <stdio.h>

static NonnullRefPtr<Gfx::Bitmap> source()
{
    // A 4x3 premultiplied BGRA bitmap: saturated and mixed colors, two translucent.
    u32 const pixels[] = {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff,
        0xff20a0e0, 0x80804020, 0xffc08040, 0x40102030,
        0xff336699, 0xff996633, 0xff000000, 0xff7f7f7f,
    };
    auto bitmap = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied, { 4, 3 }));
    for (int i = 0; i < 12; ++i)
        bitmap->scanline(i / 4)[i % 4] = pixels[i];
    return bitmap;
}

static void draw(char const* name, Gfx::ColorSpace color_space)
{
    auto target = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied, { 12, 10 }));
    auto painter = Gfx::Painter::create(target);
    auto image = Gfx::ImmutableBitmap::create(source(), move(color_space));
    painter->draw_bitmap({ 1, 1, 8, 6 }, *image, { 0, 0, 4, 3 }, Gfx::ScalingMode::NearestNeighbor, {}, 1.0f, Gfx::CompositingAndBlendingOperator::SourceOver);
    painter->draw_bitmap({ 3.5f, 5.25f, 7, 4.5f }, *image, { 0, 0, 4, 3 }, Gfx::ScalingMode::Bilinear, {}, 0.6f, Gfx::CompositingAndBlendingOperator::SourceOver);
    printf("scene %s\n", name);
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 12; ++x)
            printf("0x%08x%s", target->scanline(y)[x], x == 11 ? ",\n" : ", ");
}

int main()
{
    draw("p3", MUST(Gfx::ColorSpace::from_cicp({ Media::ColorPrimaries::SMPTE432, Media::TransferCharacteristics::SRGB, Media::MatrixCoefficients::BT709, Media::VideoFullRangeFlag::Full })));
    draw("gamma22", MUST(Gfx::ColorSpace::from_cicp({ Media::ColorPrimaries::BT709, Media::TransferCharacteristics::BT470M, Media::MatrixCoefficients::BT709, Media::VideoFullRangeFlag::Full })));
    return 0;
}
