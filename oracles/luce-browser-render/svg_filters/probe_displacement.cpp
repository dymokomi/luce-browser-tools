// Probe: the values Skia m144's raster pipeline computes for sk_displacement's arithmetic
// (unpremul of an 8-bit texel, scale * (channel - 0.5), coord + displacement), drawn into an
// F32 surface so they read back exactly.
#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkShader.h"
#include "include/core/SkSurface.h"
#include "include/core/SkColorSpace.h"
#include "include/effects/SkRuntimeEffect.h"
#include <cstdio>
#include <cstring>

static unsigned bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }

int main(int argc, char** argv)
{
    int r = atoi(argv[1]), g = atoi(argv[2]), b = atoi(argv[3]), a = atoi(argv[4]);
    float px = atof(argv[5]), py = atof(argv[6]);
    SkBitmap bm;
    bm.allocPixels(SkImageInfo::Make(1, 1, kBGRA_8888_SkColorType, kPremul_SkAlphaType, nullptr));
    *bm.getAddr32(0, 0) = (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
    auto image = bm.asImage();
    auto displ = image->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, SkSamplingOptions(SkFilterMode::kNearest));
    auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(
        "uniform shader displMap; uniform half2 scale; uniform half4 xSelect; uniform half4 ySelect;"
        "half4 main(float2 coord) {"
        "  half4 dc = unpremul(displMap.eval(coord));"
        "  half2 d = half2(dot(dc, xSelect), dot(dc, ySelect));"
        "  d = scale * (d - 0.5);"
        "  float2 c = coord + d;"
        "  return half4(coord.y, d.y, c.y, 1);"
        "}"));
    if (!effect) { printf("%s\n", err.c_str()); return 1; }
    SkRuntimeShaderBuilder builder(effect);
    builder.child("displMap") = displ;
    builder.uniform("scale") = SkV2 { 30, 30 };
    builder.uniform("xSelect") = SkV4 { 1, 0, 0, 0 };
    builder.uniform("ySelect") = SkV4 { 0, 1, 0, 0 };
    auto surface = SkSurfaces::Raster(SkImageInfo::Make(64, 64, kRGBA_F32_SkColorType, kPremul_SkAlphaType, nullptr));
    SkPaint paint;
    paint.setShader(builder.makeShader());
    paint.setBlendMode(SkBlendMode::kSrc);
    surface->getCanvas()->drawPaint(paint);
    float out[4];
    SkImageInfo info = SkImageInfo::Make(1, 1, kRGBA_F32_SkColorType, kPremul_SkAlphaType, nullptr);
    surface->readPixels(info, out, 16, (int)px, (int)py);
    printf("coord.y %.9g d.y %.9g c.y %.9g bits %08x %08x %08x\n", out[0], out[1], out[2], bits(out[0]), bits(out[1]), bits(out[2]));
    return 0;
}
