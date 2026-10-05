// skcms_oracle: dumps what skcms makes of ICC profiles, in the text format of luce-color's
// tests/icc_tool.lucb, so the two can be compared line for line (compare.py).
//
// Built twice by build.sh from the skcms of the Skia Ladybird pins: `skcms_oracle`
// (SKCMS_PORTABLE, -ffp-contract=off: the one-lane pipeline luce-color's port follows, to
// the bit) and `skcms_oracle_simd` (the NEON build Skia ships, compared within a tolerance).
//
// usage: skcms_oracle PROFILE... (prints one block a profile)
//        skcms_oracle --formats PROFILE (every pixel format through the profile)
#include "skcms.h"
#include "src/skcms_internals.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static uint32_t bits(float f) {
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}

static void print_tf(const skcms_TransferFunction* tf) {
    printf(" %08x %08x %08x %08x %08x %08x %08x", bits(tf->g), bits(tf->a), bits(tf->b), bits(tf->c), bits(tf->d), bits(tf->e), bits(tf->f));
}

static void print_curve(const char* label, int i, const skcms_Curve* c, const uint8_t* buffer) {
    printf("%s %d %u", label, i, c->table_entries);
    if (c->table_entries == 0) {
        print_tf(&c->parametric);
    } else if (c->table_8) {
        printf(" t8 %ld", (long)(c->table_8 - buffer));
    } else {
        printf(" t16 %ld", (long)(c->table_16 - buffer));
    }
    printf("\n");
}

static void print_m33(const char* label, const skcms_Matrix3x3* m) {
    printf("%s", label);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            printf(" %08x", bits(m->vals[r][c]));
    printf("\n");
}

static void print_m34(const char* label, const skcms_Matrix3x4* m) {
    printf("%s", label);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            printf(" %08x", bits(m->vals[r][c]));
    printf("\n");
}

static void print_bytes(const char* label, const uint8_t* p, size_t n) {
    printf("%s", label);
    for (size_t i = 0; i < n; i++)
        printf("%s%02x", i % 64 == 0 ? "\n  " : "", p[i]);
    printf("\n");
}

static void print_floats(const char* label, const float* p, size_t n) {
    printf("%s", label);
    for (size_t i = 0; i < n; i++)
        printf("%s%08x", i % 16 == 0 ? "\n  " : " ", bits(p[i]));
    printf("\n");
}

// The deterministic test pixels: an LCG, as icc_tool.lucb makes them.
static std::vector<uint8_t> test_pixels(size_t n) {
    std::vector<uint8_t> v(n);
    uint32_t s = 0x12345678u;
    for (size_t i = 0; i < n; i++) {
        s = s * 1664525u + 1013904223u;
        v[i] = (uint8_t)(s >> 24);
    }
    return v;
}

static void dump(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("profile %s\nmissing\n", path);
        return;
    }
    std::vector<uint8_t> data;
    uint8_t chunk[4096];
    size_t got;
    while ((got = fread(chunk, 1, sizeof chunk, f)) > 0)
        data.insert(data.end(), chunk, chunk + got);
    fclose(f);
    const char* name = strrchr(path, '/');
    printf("profile %s\n", name ? name + 1 : path);

    skcms_ICCProfile p;
    bool ok = skcms_Parse(data.data(), data.size(), &p);
    printf("parse %d\n", ok ? 1 : 0);
    if (!ok)
        return;
    const uint8_t* buf = p.buffer;
    printf("header %u %08x %08x %u\n", p.size, p.data_color_space, p.pcs, p.tag_count);
    printf("flags %d %d %d %d %d\n", p.has_trc, p.has_toXYZD50, p.has_A2B, p.has_B2A, p.has_CICP);
    if (p.has_trc)
        for (int i = 0; i < 3; i++)
            print_curve("trc", i, &p.trc[i], buf);
    if (p.has_toXYZD50)
        print_m33("xyz", &p.toXYZD50);
    if (p.has_A2B) {
        const skcms_A2B& a = p.A2B;
        printf("a2b %u %u %u %u %u %u %u", a.input_channels, a.matrix_channels, a.output_channels, a.grid_points[0], a.grid_points[1], a.grid_points[2], a.grid_points[3]);
        if (a.input_channels) {
            if (a.grid_8)
                printf(" g8 %ld", (long)(a.grid_8 - buf));
            else
                printf(" g16 %ld", (long)(a.grid_16 - buf));
        }
        printf("\n");
        for (uint32_t i = 0; i < a.input_channels; i++)
            print_curve("a2b_in", (int)i, &a.input_curves[i], buf);
        for (uint32_t i = 0; i < a.matrix_channels; i++)
            print_curve("a2b_m", (int)i, &a.matrix_curves[i], buf);
        if (a.matrix_channels)
            print_m34("a2b_matrix", &a.matrix);
        for (uint32_t i = 0; i < a.output_channels; i++)
            print_curve("a2b_out", (int)i, &a.output_curves[i], buf);
    }
    if (p.has_B2A) {
        const skcms_B2A& b = p.B2A;
        printf("b2a %u %u %u %u %u %u %u", b.input_channels, b.matrix_channels, b.output_channels, b.grid_points[0], b.grid_points[1], b.grid_points[2], b.grid_points[3]);
        if (b.output_channels) {
            if (b.grid_8)
                printf(" g8 %ld", (long)(b.grid_8 - buf));
            else
                printf(" g16 %ld", (long)(b.grid_16 - buf));
        }
        printf("\n");
        for (uint32_t i = 0; i < b.input_channels; i++)
            print_curve("b2a_in", (int)i, &b.input_curves[i], buf);
        for (uint32_t i = 0; i < b.matrix_channels; i++)
            print_curve("b2a_m", (int)i, &b.matrix_curves[i], buf);
        if (b.matrix_channels)
            print_m34("b2a_matrix", &b.matrix);
        for (uint32_t i = 0; i < b.output_channels; i++)
            print_curve("b2a_out", (int)i, &b.output_curves[i], buf);
    }
    if (p.has_CICP)
        printf("cicp %u %u %u %u\n", p.CICP.color_primaries, p.CICP.transfer_characteristics, p.CICP.matrix_coefficients, p.CICP.video_full_range_flag);

    skcms_Matrix3x3 chad;
    if (skcms_GetCHAD(&p, &chad))
        print_m33("chad", &chad);
    float wtpt[3];
    if (skcms_GetWTPT(&p, wtpt))
        printf("wtpt %08x %08x %08x\n", bits(wtpt[0]), bits(wtpt[1]), bits(wtpt[2]));
    printf("channels %d\n", skcms_GetInputChannelCount(&p));

    if (p.has_trc) {
        for (int i = 0; i < 3; i++) {
            skcms_TransferFunction tf;
            float err;
            if (skcms_ApproximateCurve(&p.trc[i], &tf, &err)) {
                printf("approx %d 1", i);
                print_tf(&tf);
                printf(" %08x\n", bits(err));
            } else {
                printf("approx %d 0\n", i);
            }
        }
        printf("inverse_srgb %d\n", skcms_TRCs_AreApproximateInverse(&p, skcms_sRGB_Inverse_TransferFunction()) ? 1 : 0);
    }
    printf("equal_srgb %d\n", skcms_ApproximatelyEqualProfiles(&p, skcms_sRGB_profile()) ? 1 : 0);

    skcms_ICCProfile usable = p;
    bool u = skcms_MakeUsableAsDestination(&usable);
    printf("usable %d", u ? 1 : 0);
    if (u && !usable.has_B2A)
        for (int i = 0; i < 3; i++)
            print_tf(&usable.trc[i].parametric);
    printf("\n");
    skcms_ICCProfile single = p;
    bool s = skcms_MakeUsableAsDestinationWithSingleCurve(&single);
    printf("single %d", s ? 1 : 0);
    if (s)
        print_tf(&single.trc[0].parametric);
    printf("\n");

    // The profile's pixels to sRGB and to XYZ D50, in 8-bit and float, straight and
    // premultiplied; and sRGB to the profile when it can be a destination.
    bool cmyk = p.data_color_space == skcms_Signature_CMYK;
    skcms_PixelFormat in = cmyk ? skcms_PixelFormat_RGBA_8888 : skcms_PixelFormat_RGB_888;
    size_t in_bpp = cmyk ? 4 : 3;
    size_t n = 256;
    std::vector<uint8_t> src = test_pixels(n * 4);
    std::vector<uint8_t> out8(n * 4);
    std::vector<float> outf(n * 4);
    (void)in_bpp;
    bool t = skcms_Transform(src.data(), in, skcms_AlphaFormat_Unpremul, &p, out8.data(), skcms_PixelFormat_RGBA_8888, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(), n);
    printf("to_srgb8 %d", t ? 1 : 0);
    if (t)
        print_bytes("", out8.data(), n * 4);
    else
        printf("\n");
    t = skcms_Transform(src.data(), in, skcms_AlphaFormat_Unpremul, &p, outf.data(), skcms_PixelFormat_RGBA_ffff, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(), n);
    printf("to_srgbf %d", t ? 1 : 0);
    if (t)
        print_floats("", outf.data(), n * 4);
    else
        printf("\n");
    t = skcms_Transform(src.data(), cmyk ? in : skcms_PixelFormat_RGBA_8888, cmyk ? skcms_AlphaFormat_Unpremul : skcms_AlphaFormat_PremulAsEncoded, &p, out8.data(), skcms_PixelFormat_BGRA_8888, skcms_AlphaFormat_PremulAsEncoded, skcms_sRGB_profile(), n);
    printf("to_srgb_premul %d", t ? 1 : 0);
    if (t)
        print_bytes("", out8.data(), n * 4);
    else
        printf("\n");
    t = skcms_Transform(src.data(), in, skcms_AlphaFormat_Opaque, &p, outf.data(), skcms_PixelFormat_RGB_fff, skcms_AlphaFormat_Opaque, skcms_XYZD50_profile(), n);
    printf("to_xyz %d", t ? 1 : 0);
    if (t)
        print_floats("", outf.data(), n * 3);
    else
        printf("\n");
    if (u) {
        t = skcms_Transform(src.data(), skcms_PixelFormat_RGBA_8888, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(), out8.data(), cmyk ? skcms_PixelFormat_RGBA_8888 : skcms_PixelFormat_RGB_888, skcms_AlphaFormat_Unpremul, &usable, n);
        printf("from_srgb %d", t ? 1 : 0);
        if (t)
            print_bytes("", out8.data(), n * (cmyk ? 4 : 3));
        else
            printf("\n");
    }
}

// Every source format to RGBA_ffff and every destination format from it, through `path`'s
// profile to sRGB and back.
static void formats(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f)
        return;
    std::vector<uint8_t> data(1 << 20);
    data.resize(fread(data.data(), 1, data.size(), f));
    fclose(f);
    skcms_ICCProfile p;
    if (!skcms_Parse(data.data(), data.size(), &p))
        return;
    size_t n = 64;
    std::vector<uint8_t> src = test_pixels(n * 16);
    // Floats in 0..1.25 with signs, for the float and half sources.
    std::vector<float> fsrc(n * 4);
    for (size_t i = 0; i < n * 4; i++)
        fsrc[i] = ((int)src[i] - 32) / 180.0f;
    std::vector<uint8_t> out(n * 16);
    for (int fmt = 0; fmt <= skcms_PixelFormat_BGRA_10101010_XR; fmt++) {
        const void* in = src.data();
        if (fmt >= skcms_PixelFormat_RGB_fff && fmt <= skcms_PixelFormat_BGRA_ffff)
            in = fsrc.data();
        for (int alpha = 0; alpha < 3; alpha++) {
            bool t = skcms_Transform(in, (skcms_PixelFormat)fmt, (skcms_AlphaFormat)alpha, &p, out.data(), skcms_PixelFormat_RGBA_ffff, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(), n);
            printf("load %d %d %d", fmt, alpha, t ? 1 : 0);
            if (t)
                print_floats("", (const float*)out.data(), n * 4);
            else
                printf("\n");
        }
    }
    skcms_ICCProfile usable = p;
    if (!skcms_MakeUsableAsDestination(&usable))
        usable = *skcms_sRGB_profile();
    for (int fmt = 0; fmt <= skcms_PixelFormat_BGRA_10101010_XR; fmt++) {
        for (int alpha = 0; alpha < 3; alpha++) {
            memset(out.data(), 0, out.size());
            bool t = skcms_Transform(fsrc.data(), skcms_PixelFormat_RGBA_ffff, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(), out.data(), (skcms_PixelFormat)fmt, (skcms_AlphaFormat)alpha, &usable, n);
            printf("store %d %d %d", fmt, alpha, t ? 1 : 0);
            if (t)
                print_bytes("", out.data(), n * 16);
            else
                printf("\n");
        }
    }
}

int main(int argc, char** argv) {
    if (argc >= 3 && strcmp(argv[1], "--formats") == 0) {
        formats(argv[2]);
        return 0;
    }
    for (int i = 1; i < argc; i++)
        dump(argv[i]);
    return 0;
}
