// luce-png's APNG oracle: what Ladybird's PNGImageDecoderPlugin
// (Libraries/LibGfx/ImageFormats/PNGLoader.cpp) makes of a PNG with libpng 1.6.50 and its APNG
// patch (Ladybird's vcpkg build), frame by frame: initialize() (png_read_info and the
// transforms to 8-bit RGBA), then read_frames(): for an acTL, png_read_frame_head and
// png_read_image for each of acTL's frames (the hidden default image counted), each frame
// with an fcTL composited into the canvas and pushed, then disposed. Ladybird composites with
// its Painter, Skia's highp raster pipeline into a straight-alpha surface; this oracle draws
// with the same float steps (skia_draw), which luce-png follows, and ladybird/ dumps
// Ladybird's own frames to check them. It prints
//
//   PATH init-error                                   initialize() failed: no image
//   PATH ok WIDTH HEIGHT FRAMES PLAYS                 FRAMES the acTL count read (hidden
//                                                     default image included), - for a still PNG
//   frame K X Y W H NUM DEN DISPOSE BLEND HASH        each frame pushed: its fcTL and the
//                                                     FNV-1a 64 hash of the canvas as RGBA
//   PATH fallback PUSHED                              read_frames failed after PUSHED frames
//
// With `-o DIR` it also writes each frame pushed, as RGBA, to DIR/K.rgba (one file at a time).
//
// Usage: apng_oracle [-o DIR] FILE...
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void quiet(png_structp p, png_const_charp m) {
    (void)p;
    if (getenv("APNG_ORACLE_VERBOSE")) fprintf(stderr, "  warning: %s\n", m);
}
static int verbose = 0;
static const char* out_dir = NULL;
static void fail(png_structp p, png_const_charp m) {
    if (verbose) fprintf(stderr, "  error: %s\n", m);
    png_longjmp(p, 1);
}

typedef struct {
    const uint8_t* data;
    size_t size;
} Source;

static void read_data(png_structp png, png_bytep out, png_size_t length) {
    Source* source = (Source*)png_get_io_ptr(png);
    if (source->size < length) png_error(png, "Read error");
    memcpy(out, source->data, length);
    source->data += length;
    source->size -= length;
}

static uint64_t fnv(const uint8_t* d, size_t n) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 1099511628211ull; }
    return h;
}

// 1/255 as a float, Skia's from_byte scale.
static float byte_scale(void) {
    uint32_t bits = 0x3b808081;
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

// Skia's to_unorm(v, 255): clamped, scaled, rounded to nearest even.
static uint8_t to_unorm(float v) {
    float clamped = !(v > 0.0f) ? 0.0f : v > 1.0f ? 1.0f : v;
    return (uint8_t)nearbyintf(clamped * 255.0f);
}

// A straight RGBA pixel drawn onto another through Skia's highp pipeline, as Ladybird's
// Painter draws into a straight-alpha surface: premultiplied, Copy or SourceOver (a fused
// multiply-add, as on arm64), unpremultiplied, stored.
static void skia_draw(uint8_t* dp, const uint8_t* sp, int over) {
    float scale = byte_scale();
    float sa = sp[3] * scale;
    float a = sa;
    float color[3];
    for (int c = 0; c < 3; ++c) color[c] = sp[c] * scale * sa;
    if (over) {
        float da = dp[3] * scale;
        float inverse = 1.0f - sa;
        for (int c = 0; c < 3; ++c) color[c] = fmaf(dp[c] * scale * da, inverse, color[c]);
        a = fmaf(da, inverse, sa);
    }
    float reciprocal = 1.0f / a;
    float unpremul = reciprocal < INFINITY ? reciprocal : 0.0f;
    for (int c = 0; c < 3; ++c) dp[c] = to_unorm(color[c] * unpremul);
    dp[3] = to_unorm(a);
}

static void run(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf("%s init-error\n", path); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* bytes = malloc(size > 0 ? size : 1);
    size_t got = fread(bytes, 1, size, f);
    fclose(f);
    Source source = {bytes, got};

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, fail, quiet);
    png_infop info = png_create_info_struct(png);
    uint8_t* volatile canvas = NULL;
    uint8_t* volatile frame = NULL;
    uint8_t* volatile previous = NULL;
    png_bytep* volatile rows = NULL;
    volatile int pushed = 0;
    volatile int initialized = 0;
    if (setjmp(png_jmpbuf(png))) {
        if (!initialized) printf("%s init-error\n", path);
        else printf("%s fallback %d\n", path, pushed);
        goto done;
    }
    png_set_read_fn(png, &source, read_data);
    png_read_info(png, info);
    png_uint_32 width, height;
    int bit_depth, color_type, interlace;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, &interlace, NULL, NULL);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
    if (interlace != PNG_INTERLACE_NONE) png_set_interlace_handling(png);
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    initialized = 1;
    png_read_update_info(png, info);

    png_uint_32 frame_count = 0, plays = 0;
    size_t canvas_size = (size_t)width * height * 4;
    canvas = calloc(canvas_size, 1);
    frame = calloc(canvas_size, 1);
    previous = calloc(canvas_size, 1);
    rows = malloc(sizeof(png_bytep) * height);
    if (png_get_acTL(png, info, &frame_count, &plays)) {
        printf("%s ok %u %u %u %u\n", path, width, height, frame_count - (png_get_first_frame_is_hidden(png, info) ? 1 : 0), plays);
        png_set_acTL(png, info, frame_count, plays);
        for (png_uint_32 index = 0; index < frame_count; ++index) {
            png_read_frame_head(png, info);
            png_uint_32 w = 0, h = 0, x = 0, y = 0;
            png_uint_16 num = 0, den = 0;
            png_byte dispose = PNG_DISPOSE_OP_NONE, blend = PNG_BLEND_OP_SOURCE;
            int has_fctl = png_get_valid(png, info, PNG_INFO_fcTL) != 0;
            if (has_fctl) {
                png_get_next_frame_fcTL(png, info, &w, &h, &x, &y, &num, &den, &dispose, &blend);
            } else {
                w = png_get_image_width(png, info);
                h = png_get_image_height(png, info);
            }
            for (png_uint_32 r = 0; r < h; ++r) rows[r] = frame + (size_t)r * w * 4;
            png_read_image(png, rows);
            if (!has_fctl) continue;
            if (dispose == PNG_DISPOSE_OP_PREVIOUS) memcpy(previous, canvas, canvas_size);
            for (png_uint_32 r = 0; r < h; ++r) {
                for (png_uint_32 c = 0; c < w; ++c) {
                    uint8_t* dp = canvas + ((size_t)(y + r) * width + x + c) * 4;
                    const uint8_t* sp = frame + ((size_t)r * w + c) * 4;
                    skia_draw(dp, sp, blend == PNG_BLEND_OP_OVER);
                }
            }
            printf("frame %d %u %u %u %u %u %u %u %u %016llx\n", pushed, x, y, w, h, num, den, dispose, blend, (unsigned long long)fnv(canvas, canvas_size));
            if (out_dir) {
                char name[4096];
                snprintf(name, sizeof name, "%s/%d.rgba", out_dir, pushed);
                FILE* out = fopen(name, "wb");
                if (out) { fwrite(canvas, 1, canvas_size, out); fclose(out); }
            }
            ++pushed;
            for (png_uint_32 r = 0; r < h; ++r) {
                uint8_t* dp = canvas + ((size_t)(y + r) * width + x) * 4;
                if (dispose == PNG_DISPOSE_OP_BACKGROUND) memset(dp, 0, (size_t)w * 4);
                else if (dispose == PNG_DISPOSE_OP_PREVIOUS)
                    for (png_uint_32 c = 0; c < w; ++c) skia_draw(dp + c * 4, previous + ((size_t)(y + r) * width + x + c) * 4, 0);
            }
        }
    } else {
        printf("%s ok %u %u - -\n", path, width, height);
        for (png_uint_32 r = 0; r < height; ++r) rows[r] = canvas + (size_t)r * width * 4;
        png_read_image(png, rows);
        printf("frame 0 0 0 %u %u 0 0 0 0 %016llx\n", width, height, (unsigned long long)fnv(canvas, canvas_size));
    }
done:
    png_destroy_read_struct(&png, &info, NULL);
    free(canvas);
    free(frame);
    free(previous);
    free(rows);
    free(bytes);
}

int main(int argc, char** argv) {
    verbose = getenv("APNG_ORACLE_VERBOSE") != NULL;
    int first = 1;
    if (argc >= 3 && !strcmp(argv[1], "-o")) {
        out_dir = argv[2];
        first = 3;
    }
    for (int i = first; i < argc; ++i) run(argv[i]);
    return 0;
}
