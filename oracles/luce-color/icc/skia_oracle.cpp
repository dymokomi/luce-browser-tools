// skia_oracle: what Skia m144 (the libskia Ladybird builds) makes of ICC profiles, in the
// format of luce-color's `icc_tool --spaces`:
//
//   profile NAME
//   space 0|1 TF(7 hex) MATRIX(9 hex)      SkColorSpace::Make(profile)
//   ladybird 0|1 TF MATRIX                 Gfx::ColorSpace::load_from_icc_bytes's result,
//                                          with its skcms_ApproximateCurve fallback
//   draw 0|1 BYTES                         the test pixels as an N32 premultiplied image in
//                                          that color space, drawn 1:1 into an sRGB raster
//                                          surface (SkImageShader + SkColorSpaceXformSteps),
//                                          read back as RGBA 8888 premultiplied
//
// usage: skia_oracle PROFILE...
//        skia_oracle --draws   (the draw of named color spaces: NAME FNV1A and the first
//                               pixels, for luce-browser-render's raster color_xform test)
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "modules/skcms/skcms.h"

#include <stdint.h>
#include <utility>
#include <stdio.h>
#include <string.h>
#include <vector>

static uint32_t bits(float f) {
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}

static void print_space(const char* label, SkColorSpace* cs) {
    printf("%s %d", label, cs ? 1 : 0);
    if (cs) {
        skcms_TransferFunction tf;
        cs->transferFn(&tf);
        skcms_Matrix3x3 m;
        cs->toXYZD50(&m);
        printf(" %08x %08x %08x %08x %08x %08x %08x", bits(tf.g), bits(tf.a), bits(tf.b), bits(tf.c), bits(tf.d), bits(tf.e), bits(tf.f));
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++)
                printf(" %08x", bits(m.vals[r][c]));
    }
    printf("\n");
}

static std::vector<uint8_t> test_pixels(size_t n) {
    std::vector<uint8_t> v(n);
    uint32_t s = 0x12345678u;
    for (size_t i = 0; i < n; i++) {
        s = s * 1664525u + 1013904223u;
        v[i] = (uint8_t)(s >> 24);
    }
    return v;
}

// The test pixels drawn 1:1 from color space `cs` into an sRGB raster surface.
static std::vector<uint8_t> draw(sk_sp<SkColorSpace> cs) {
    const int w = 16, h = 16;
    std::vector<uint8_t> px = test_pixels(w * h * 4);
    for (int k = 0; k < w * h; k++) {
        uint8_t a = (k % 4 == 0) ? 255 : px[k * 4 + 3];
        for (int c = 0; c < 3; c++)
            px[k * 4 + c] = (uint8_t)((px[k * 4 + c] * a + 127) / 255);
        px[k * 4 + 3] = a;
    }
    SkBitmap bitmap;
    bitmap.installPixels(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, cs), px.data(), w * 4);
    bitmap.setImmutable();
    sk_sp<SkImage> image = bitmap.asImage();
    auto info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
    sk_sp<SkSurface> surface = SkSurfaces::Raster(info);
    surface->getCanvas()->clear(SK_ColorTRANSPARENT);
    surface->getCanvas()->drawImage(image, 0, 0, SkSamplingOptions(), nullptr);
    std::vector<uint8_t> out(w * h * 4);
    surface->readPixels(info, out.data(), w * 4, 0, 0);
    return out;
}

static uint32_t fnv(const uint8_t* d, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 16777619u; }
    return h;
}

static void draws() {
    skcms_Matrix3x3 swapped = SkNamedGamut::kSRGB;
    for (int r = 0; r < 3; r++)
        std::swap(swapped.vals[r][0], swapped.vals[r][1]);
    skcms_Matrix3x3 prophoto;
    SkNamedPrimaries::kProPhotoRGB.toXYZD50(&prophoto);
    struct { const char* name; sk_sp<SkColorSpace> cs; } cases[] = {
        { "swap", SkColorSpace::MakeRGB(SkNamedTransferFn::kSRGB, swapped) },
        { "p3", SkColorSpace::MakeRGB(SkNamedTransferFn::kSRGB, SkNamedGamut::kDisplayP3) },
        { "adobe", SkColorSpace::MakeRGB(SkNamedTransferFn::k2Dot2, SkNamedGamut::kAdobeRGB) },
        { "rec2020-pq", SkColorSpace::MakeRGB(SkNamedTransferFn::kPQ, SkNamedGamut::kRec2020) },
        { "rec2020-hlg", SkColorSpace::MakeRGB(SkNamedTransferFn::kHLG, SkNamedGamut::kRec2020) },
        { "linear", SkColorSpace::MakeSRGBLinear() },
        { "prophoto", SkColorSpace::MakeRGB(SkNamedTransferFn::kProPhotoRGB, prophoto) },
    };
    for (auto& c : cases) {
        std::vector<uint8_t> out = draw(c.cs);
        printf("%s %08x", c.name, fnv(out.data(), out.size()));
        for (int k = 0; k < 16; k++)
            printf(" %d", out[k]);
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "--draws") == 0) {
        draws();
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        FILE* f = fopen(argv[i], "rb");
        if (!f)
            continue;
        std::vector<uint8_t> data(1 << 20);
        data.resize(fread(data.data(), 1, data.size(), f));
        fclose(f);
        const char* name = strrchr(argv[i], '/');
        printf("profile %s\n", name ? name + 1 : argv[i]);
        skcms_ICCProfile p;
        if (!skcms_Parse(data.data(), data.size(), &p)) {
            printf("parse 0\n");
            continue;
        }
        sk_sp<SkColorSpace> cs = SkColorSpace::Make(p);
        print_space("space", cs.get());
        // Gfx::ColorSpace::load_from_icc_bytes (Libraries/LibGfx/ColorSpace.cpp).
        sk_sp<SkColorSpace> ladybird = cs;
        if (!ladybird && p.has_trc && p.has_toXYZD50) {
            skcms_TransferFunction tf;
            float max_error;
            if (skcms_ApproximateCurve(&p.trc[0], &tf, &max_error))
                ladybird = SkColorSpace::MakeRGB(tf, p.toXYZD50);
        }
        print_space("ladybird", ladybird.get());

        // 16x16 premultiplied pixels: the LCG's colors, alpha from the LCG with every
        // fourth pixel opaque.
        const int w = 16, h = 16;
        std::vector<uint8_t> px = test_pixels(w * h * 4);
        for (int k = 0; k < w * h; k++) {
            uint8_t a = (k % 4 == 0) ? 255 : px[k * 4 + 3];
            for (int c = 0; c < 3; c++)
                px[k * 4 + c] = (uint8_t)((px[k * 4 + c] * a + 127) / 255);
            px[k * 4 + 3] = a;
        }
        SkImageInfo info = SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, ladybird);
        SkBitmap bitmap;
        bitmap.installPixels(info, px.data(), w * 4);
        bitmap.setImmutable();
        sk_sp<SkImage> image = bitmap.asImage();
        sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()));
        surface->getCanvas()->clear(SK_ColorTRANSPARENT);
        surface->getCanvas()->drawImage(image, 0, 0, SkSamplingOptions(), nullptr);
        std::vector<uint8_t> out(w * h * 4);
        surface->readPixels(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB()), out.data(), w * 4, 0, 0);
        printf("draw 1");
        for (size_t k = 0; k < out.size(); k++)
            printf("%s%02x", k % 64 == 0 ? "\n  " : "", out[k]);
        printf("\n");
    }
    return 0;
}
