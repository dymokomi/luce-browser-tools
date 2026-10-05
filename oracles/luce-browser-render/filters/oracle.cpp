// Oracle for display_list's cpu_filter: Gfx::Filter graphs evaluated by Skia m144's raster
// SkCanvas (saveLayer with an image filter, or a backdrop), printed as 0xAARRGGBB premul pixels.
#include <AK/Function.h>
#include <AK/Vector.h>
#include <core/SkBitmap.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ColorSpace.h>
#include <LibGfx/Filter.h>
#include <LibGfx/FilterImpl.h>
#include <LibGfx/ImmutableBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkImage.h>
#include <core/SkSurface.h>
#include <effects/SkLumaColorFilter.h>
#include <modules/skcms/skcms.h>
#include <stdio.h>

using Gfx::Filter;

static uint32_t pattern_pixel(int i, int j, int w, int h)
{
    // Unpremultiplied RGBA, then premultiplied (rounding as SkPremultiplyARGBInline).
    int r = (i * 255) / (w - 1), g = (j * 255) / (h - 1), b = 255 - (i + j) * 13, a = 255 - ((i * 7 + j * 11) % 5) * 40;
    if (b < 0) b = 0;
    auto mul = [](int c, int a) { int prod = c * a + 128; return (prod + (prod >> 8)) >> 8; };
    return (uint32_t(a) << 24) | (uint32_t(mul(r, a)) << 16) | (uint32_t(mul(g, a)) << 8) | uint32_t(mul(b, a));
}

static sk_sp<SkImage> pattern_image(int w, int h)
{
    SkBitmap bm;
    bm.allocPixels(SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()));
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            *bm.getAddr32(i, j) = pattern_pixel(i, j, w, h);
    bm.setImmutable();
    return bm.asImage();
}

static NonnullRefPtr<Gfx::ImmutableBitmap> pattern_bitmap(int w, int h, Optional<Gfx::ColorSpace> color_space = {})
{
    auto bitmap = MUST(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, Gfx::AlphaType::Premultiplied, { w, h }));
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            bitmap->scanline(j)[i] = pattern_pixel(i, j, w, h);
    if (color_space.has_value())
        return Gfx::ImmutableBitmap::create(bitmap, color_space.release_value());
    return Gfx::ImmutableBitmap::create(bitmap);
}

struct Scene {
    char const* name;
    int w { 24 }, h { 24 };
    SkMatrix ctm { SkMatrix::I() };
    SkIRect clip { SkIRect::MakeWH(1000, 1000) };
    float alpha { 1 };
    bool luma { false };
    bool backdrop { false };
    // Restore with source-over (as Ladybird's ApplyEffects layers do) instead of kSrc: the same
    // composite onto the transparent canvas, but FilterResult::draw takes drawSpecial's path
    // rather than filling the device with the result's shader.
    bool src_over { false };
};

// Content: the 10x8 pattern at (7, 8) and two solid rects, drawn under the CTM (non-AA).
static void content(SkCanvas* c)
{
    c->drawImage(pattern_image(10, 8), 7, 8);
    SkPaint p;
    p.setColor(0xFF20C040);
    c->drawRect(SkRect::MakeXYWH(3, 3, 4, 3), p);
    p.setColor(0x80E03010);
    c->drawRect(SkRect::MakeXYWH(14, 15, 5, 6), p);
}

static void run(Scene const& s, Filter const* filter)
{
    auto surface = SkSurfaces::Raster(SkImageInfo::Make(s.w, s.h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()));
    auto* c = surface->getCanvas();
    c->clear(SK_ColorTRANSPARENT);
    c->clipIRect(s.clip);
    c->setMatrix(s.ctm);
    SkPaint paint;
    if (!s.src_over)
        paint.setBlendMode(SkBlendMode::kSrc);
    if (s.alpha < 1)
        paint.setAlphaf(s.alpha);
    if (s.luma)
        paint.setColorFilter(SkLumaColorFilter::Make());
    if (s.backdrop) {
        content(c);
        c->saveLayer(SkCanvas::SaveLayerRec(nullptr, &paint, filter ? filter->impl().filter.get() : nullptr, 0));
        c->restore();
    } else {
        if (filter)
            paint.setImageFilter(filter->impl().filter);
        c->saveLayer(nullptr, &paint);
        content(c);
        c->restore();
    }
    SkPixmap pm;
    surface->peekPixels(&pm);
    printf("scene %s %d %d\n", s.name, s.w, s.h);
    for (int y = 0; y < s.h; ++y) {
        for (int x = 0; x < s.w; ++x)
            printf("%08x%c", *pm.addr32(x, y), x + 1 == s.w ? '\n' : ' ');
    }
}

static void scene(Scene s, Filter const& f) { run(s, &f); }

int main()
{
    // The inverse sRGB transfer function (LinearToSRGBGamma's encode step).
    skcms_TransferFunction tf;
    SkColorSpace::MakeSRGB()->invTransferFn(&tf);
    uint32_t bits[7];
    memcpy(bits, &tf, sizeof(bits));
    skcms_TransferFunction srgb;
    SkColorSpace::MakeSRGB()->transferFn(&srgb);
    uint32_t sbits[7];
    memcpy(sbits, &srgb, sizeof(sbits));
    printf("srgb %08x %08x %08x %08x %08x %08x %08x\n", sbits[0], sbits[1], sbits[2], sbits[3], sbits[4], sbits[5], sbits[6]);
    printf("inv_srgb %08x %08x %08x %08x %08x %08x %08x\n", bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6]);
    SkMatrix scale2 = SkMatrix::Translate(1, 2.3f);
    scale2.preScale(2, 1.25f);

    run({ .name = "identity" }, nullptr);
    run({ .name = "identity_scale", .ctm = scale2 }, nullptr);
    run({ .name = "content", .w = 48, .h = 48 }, nullptr);
    run({ .name = "content_scale", .w = 48, .h = 48, .ctm = scale2 }, nullptr);
    SkMatrix rotate = SkMatrix::RotateDeg(30, { 12, 12 });
    run({ .name = "content_rotate", .w = 48, .h = 48, .ctm = rotate }, nullptr);
    scene({ .name = "offset_rotate", .ctm = rotate, .clip = SkIRect::MakeLTRB(2, 3, 21, 22) }, Filter::offset(2, 1));
    scene({ .name = "sepia_rotate", .ctm = rotate }, Filter::color(Gfx::ColorFilterType::Sepia, 0.8f));
    scene({ .name = "blur_1_5" }, Filter::blur(1.5f, 1.5f));
    scene({ .name = "blur_2_5" }, Filter::blur(2.5f, 2.5f));
    scene({ .name = "blur_4" }, Filter::blur(4, 4));
    scene({ .name = "blur_0_3" }, Filter::blur(0, 3));
    scene({ .name = "blur_clip", .clip = SkIRect::MakeLTRB(5, 6, 18, 20) }, Filter::blur(3, 2));
    scene({ .name = "blur_scale", .ctm = scale2, .clip = SkIRect::MakeLTRB(2, 2, 22, 22) }, Filter::blur(1, 2));
    scene({ .name = "blur_alpha", .alpha = 0.6f }, Filter::blur(2, 2));
    scene({ .name = "blur_12", .w = 32, .h = 32 }, Filter::blur(12, 12));
    scene({ .name = "blur_40", .w = 32, .h = 32 }, Filter::blur(40, 30));
    scene({ .name = "blur_300" }, Filter::blur(300, 300));
    scene({ .name = "grayscale" }, Filter::color(Gfx::ColorFilterType::Grayscale, 0.7f));
    scene({ .name = "chain" }, Filter::color(Gfx::ColorFilterType::Brightness, 1.3f, Filter::color(Gfx::ColorFilterType::Sepia, 0.5f)));
    scene({ .name = "chain_alpha_luma", .alpha = 0.5f, .luma = true }, Filter::hue_rotate(40, Filter::saturate(1.7f)));
    scene({ .name = "invert_opacity" }, Filter::color(Gfx::ColorFilterType::Opacity, 0.5f, Filter::color(Gfx::ColorFilterType::Invert, 0.8f)));
    scene({ .name = "contrast_blur" }, Filter::blur(1.5f, 1.5f, Filter::color(Gfx::ColorFilterType::Contrast, 1.6f)));
    {
        u8 table[256];
        for (int i = 0; i < 256; ++i)
            table[i] = 255 - (i * i) / 255;
        scene({ .name = "color_table" }, Filter::color_table({}, ReadonlyBytes { table, 256 }, {}, ReadonlyBytes { table, 256 }));
    }
    {
        float m[20] = { 0.5f, 0, 0, 0, 0.1f, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0.25f };
        scene({ .name = "matrix_fills", .clip = SkIRect::MakeLTRB(2, 1, 20, 22) }, Filter::color_matrix(m));
    }
    scene({ .name = "drop_shadow" }, Filter::drop_shadow(3.5f, 2, 2, Gfx::Color(10, 20, 200, 180)));
    scene({ .name = "drop_shadow_scale", .ctm = scale2 }, Filter::drop_shadow(-1, 1.5f, 1, Gfx::Color(0, 0, 0, 255)));
    scene({ .name = "offset_int" }, Filter::offset(3, -2));
    scene({ .name = "offset_frac" }, Filter::offset(2.5f, 1.25f));
    {
        Vector<Optional<Filter>> inputs;
        inputs.append(Filter::offset(4, 1));
        inputs.append(Optional<Filter> {});
        inputs.append(Filter::flood(Gfx::Color(255, 0, 0, 255), 0.25f));
        scene({ .name = "merge", .clip = SkIRect::MakeLTRB(1, 2, 23, 21) }, Filter::merge(inputs));
    }
    scene({ .name = "erode" }, Filter::erode(1, 1));
    scene({ .name = "dilate" }, Filter::dilate(2, 1));
    scene({ .name = "flood", .clip = SkIRect::MakeLTRB(3, 4, 12, 9) }, Filter::flood(Gfx::Color(30, 60, 90, 255), 0.7f));
    scene({ .name = "fractal_noise" }, Filter::turbulence(Gfx::TurbulenceType::FractalNoise, 0.15f, 0.2f, 2, 3, { 0, 0 }));
    scene({ .name = "turbulence_stitch" }, Filter::turbulence(Gfx::TurbulenceType::Turbulence, 0.12f, 0.09f, 3, 7.5f, { 13, 11 }));
    scene({ .name = "turbulence_scale", .ctm = scale2 }, Filter::turbulence(Gfx::TurbulenceType::Turbulence, 0.2f, 0.3f, 1, 0, { 0, 0 }));
    {
        auto noise = Filter::turbulence(Gfx::TurbulenceType::FractalNoise, 0.2f, 0.2f, 1, 2, { 0, 0 });
        scene({ .name = "displacement" }, Filter::displacement_map({}, noise, 6, Gfx::ChannelSelector::Red, Gfx::ChannelSelector::Alpha));
    }
    {
        auto flood = Filter::flood(Gfx::Color(200, 100, 50, 255), 0.8f);
        scene({ .name = "arithmetic" }, Filter::arithmetic({}, flood, 0.5f, 0.3f, 0.6f, 0));
        scene({ .name = "arithmetic_k4", .clip = SkIRect::MakeLTRB(0, 0, 20, 20) }, Filter::arithmetic({}, Filter::offset(2, 2), 0.2f, 0.4f, 0.5f, 0.1f));
    }
    {
        auto image = pattern_bitmap(10, 8);
        scene({ .name = "image_nearest" }, Filter::image(*image, { 1, 1, 6, 5 }, { 3, 2, 12, 10 }, Gfx::ScalingMode::NearestNeighbor));
        scene({ .name = "image_bilinear" }, Filter::image(*image, { 0, 0, 10, 8 }, { 2, 3, 15, 13 }, Gfx::ScalingMode::Bilinear));
        scene({ .name = "image_offset" }, Filter::image(*image, { 0, 0, 10, 8 }, { 5, 4, 10, 8 }, Gfx::ScalingMode::Bilinear));
        auto fg = Filter::image(*image, { 0, 0, 10, 8 }, { 10, 11, 10, 8 }, Gfx::ScalingMode::NearestNeighbor);
        using Op = Gfx::CompositingAndBlendingOperator;
        struct { char const* name; Op op; } modes[] = {
            { "blend_normal", Op::Normal }, { "blend_multiply", Op::Multiply }, { "blend_screen", Op::Screen },
            { "blend_overlay", Op::Overlay }, { "blend_darken", Op::Darken }, { "blend_lighten", Op::Lighten },
            { "blend_color_dodge", Op::ColorDodge }, { "blend_color_burn", Op::ColorBurn }, { "blend_hard_light", Op::HardLight },
            { "blend_soft_light", Op::SoftLight }, { "blend_difference", Op::Difference }, { "blend_exclusion", Op::Exclusion },
            { "blend_hue", Op::Hue }, { "blend_saturation", Op::Saturation }, { "blend_color", Op::Color },
            { "blend_luminosity", Op::Luminosity }, { "blend_source_in", Op::SourceIn }, { "blend_destination_out", Op::DestinationOut },
            { "blend_xor", Op::Xor }, { "blend_lighter", Op::Lighter }, { "blend_plus_darker", Op::PlusDarker },
            { "blend_plus_lighter", Op::PlusLighter }, { "blend_copy", Op::Copy }, { "blend_clear", Op::Clear },
        };
        for (auto& m : modes)
            scene({ .name = m.name }, Filter::blend({}, fg, m.op));
    }
    scene({ .name = "compose" }, Filter::compose(Filter::blur(1, 1), Filter::color(Gfx::ColorFilterType::Grayscale, 1)));
    scene({ .name = "backdrop_blur", .clip = SkIRect::MakeLTRB(0, 4, 16, 18), .backdrop = true }, Filter::blur(2, 2));
    scene({ .name = "backdrop_invert", .clip = SkIRect::MakeLTRB(6, 6, 20, 20), .backdrop = true }, Filter::color(Gfx::ColorFilterType::Invert, 1));
    // Displacements that land on integer coordinates (a black map, scale 1: every pixel
    // samples the color input at (x, y), which nearest sampling's roundDownAtInteger takes
    // from (x - 1, y - 1)), and the Screenshot test's turbulence displacements.
    scene({ .name = "displacement_integer" }, Filter::displacement_map({}, Filter::flood(Gfx::Color(0, 0, 0, 255), 1), 1, Gfx::ChannelSelector::Red, Gfx::ChannelSelector::Red));
    scene({ .name = "displacement_turbulence", .w = 48, .h = 48 }, Filter::displacement_map({}, Filter::turbulence(Gfx::TurbulenceType::Turbulence, 0.05f, 0.05f, 2, 0, { 0, 0 }), 30, Gfx::ChannelSelector::Red, Gfx::ChannelSelector::Green));
    // A layer matrix with perspective: filters whose every node takes any matrix keep the
    // whole CTM as the layer matrix (skif::Mapping::decomposeCTM), so offsets, images and
    // shaders map through it.
    SkMatrix persp = SkMatrix::MakeAll(1.1f, 0.15f, 1, -0.05f, 0.95f, 2, 0.006f, -0.004f, 1);
    run({ .name = "content_persp", .w = 48, .h = 48, .ctm = persp }, nullptr);
    scene({ .name = "persp_grayscale", .ctm = persp, .src_over = true }, Filter::color(Gfx::ColorFilterType::Grayscale, 0.7f));
    scene({ .name = "persp_offset", .ctm = persp, .src_over = true }, Filter::offset(3, 2));
    {
        auto image = pattern_bitmap(10, 8);
        scene({ .name = "persp_image", .ctm = persp, .src_over = true }, Filter::image(*image, { 0, 0, 10, 8 }, { 4, 3, 10, 8 }, Gfx::ScalingMode::Bilinear));
    }
    scene({ .name = "persp_turbulence", .ctm = persp, .src_over = true }, Filter::turbulence(Gfx::TurbulenceType::FractalNoise, 0.15f, 0.2f, 2, 3, { 0, 0 }));
    // Images in another color space (SkImageFilters::Image of an SkImage tagged with it),
    // converted to the sRGB layer after sampling: Display P3 (sRGB curve), and BT.709
    // primaries with BT.470M's 2.2 gamma.
    {
        auto p3 = pattern_bitmap(10, 8, MUST(Gfx::ColorSpace::from_cicp({ Media::ColorPrimaries::SMPTE432, Media::TransferCharacteristics::SRGB, Media::MatrixCoefficients::BT709, Media::VideoFullRangeFlag::Full })));
        scene({ .name = "image_p3_nearest" }, Filter::image(*p3, { 1, 1, 6, 5 }, { 3, 2, 12, 10 }, Gfx::ScalingMode::NearestNeighbor));
        scene({ .name = "image_p3_offset" }, Filter::image(*p3, { 0, 0, 10, 8 }, { 5, 4, 10, 8 }, Gfx::ScalingMode::Bilinear));
        auto gamma = pattern_bitmap(10, 8, MUST(Gfx::ColorSpace::from_cicp({ Media::ColorPrimaries::BT709, Media::TransferCharacteristics::BT470M, Media::MatrixCoefficients::BT709, Media::VideoFullRangeFlag::Full })));
        scene({ .name = "image_gamma_blur" }, Filter::blur(1.5f, 1.5f, Filter::image(*gamma, { 0, 0, 10, 8 }, { 5, 4, 10, 8 }, Gfx::ScalingMode::Bilinear)));
    }
    return 0;
}
