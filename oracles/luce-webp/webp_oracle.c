// luce-webp's oracle: what libwebp 1.6.0 makes of a WebP file, in the order Ladybird's
// WebPImageDecoderPlugin (Libraries/LibGfx/ImageFormats/WebPLoader.cpp) asks it:
// WebPGetInfo, WebPGetFeatures, for an animation WebPAnimDecoderNew and the demuxer's
// durations, then WebPMuxCreate for the ICC profile; then the pixels, a still image through
// WebPDecodeRGBAInto and an animation frame by frame through WebPAnimDecoderGetNext (both with
// libwebp's default options, as Ladybird decodes: fancy upsampling, no dithering). It prints
//
//   PATH ok WIDTH HEIGHT ALPHA ANIMATED FRAMES LOOP BGCOLOR ICC
//   frame INDEX DURATION HASH          (or: frame INDEX error)
//
// or `PATH error STAGE`, ICC being `-` or `SIZE:HASH`, HASH the 64-bit FNV-1a of the bytes
// (the frame as straight RGBA, the whole canvas for an animation). With `-o DIR` it also
// writes frame N of the Kth file to DIR/K-N.rgba. `--bench COUNT PATH` decodes a still image
// COUNT times and prints the best time in milliseconds.
//
// Usage: webp_oracle [-o DIR] FILE...  |  webp_oracle --bench COUNT FILE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "src/webp/decode.h"
#include "src/webp/demux.h"
#include "src/webp/mux.h"

static uint64_t fnv(const uint8_t* data, size_t size) {
    uint64_t hash = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint8_t* read_file(const char* path, size_t* size) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* data = malloc(length > 0 ? (size_t)length : 1);
    *size = fread(data, 1, (size_t)length, file);
    fclose(file);
    return data;
}

static void write_frame(const char* dir, int file_index, int frame, const uint8_t* rgba, size_t size) {
    if (!dir) return;
    char path[4096];
    snprintf(path, sizeof path, "%s/%d-%d.rgba", dir, file_index, frame);
    FILE* out = fopen(path, "wb");
    if (!out) return;
    fwrite(rgba, 1, size, out);
    fclose(out);
}

static void run(const char* path, int file_index, const char* dir) {
    size_t size = 0;
    uint8_t* data = read_file(path, &size);
    if (!data) {
        printf("%s error read\n", path);
        return;
    }
    int width = 0, height = 0;
    WebPBitstreamFeatures features;
    if (!WebPGetInfo(data, size, &width, &height)) {
        printf("%s error info\n", path);
        free(data);
        return;
    }
    if (WebPGetFeatures(data, size, &features) != VP8_STATUS_OK) {
        printf("%s error features\n", path);
        free(data);
        return;
    }
    WebPData webp_data = {data, size};
    WebPAnimDecoder* anim = NULL;
    WebPAnimInfo anim_info;
    memset(&anim_info, 0, sizeof anim_info);
    if (features.has_animation) {
        WebPAnimDecoderOptions options;
        WebPAnimDecoderOptionsInit(&options);
        options.color_mode = MODE_RGBA;
        anim = WebPAnimDecoderNew(&webp_data, &options);
        if (!anim || !WebPAnimDecoderGetInfo(anim, &anim_info)) {
            printf("%s error anim\n", path);
            if (anim) WebPAnimDecoderDelete(anim);
            free(data);
            return;
        }
    }
    WebPMux* mux = WebPMuxCreate(&webp_data, 0);
    uint32_t flags = 0;
    if (WebPMuxGetFeatures(mux, &flags) != WEBP_MUX_OK) {
        printf("%s error mux\n", path);
        WebPMuxDelete(mux);
        if (anim) WebPAnimDecoderDelete(anim);
        free(data);
        return;
    }
    char icc[64] = "-";
    if (flags & ICCP_FLAG) {
        WebPData chunk;
        if (WebPMuxGetChunk(mux, "ICCP", &chunk) != WEBP_MUX_OK) {
            printf("%s error iccp\n", path);
            WebPMuxDelete(mux);
            if (anim) WebPAnimDecoderDelete(anim);
            free(data);
            return;
        }
        snprintf(icc, sizeof icc, "%zu:%016llx", chunk.size, (unsigned long long)fnv(chunk.bytes, chunk.size));
    }
    WebPMuxDelete(mux);

    int frames = features.has_animation ? (int)anim_info.frame_count : 1;
    int loop = features.has_animation ? (int)anim_info.loop_count : 0;
    uint32_t bgcolor = features.has_animation ? anim_info.bgcolor : 0;
    printf("%s ok %d %d %d %d %d %d %08x %s\n", path, width, height, features.has_alpha, features.has_animation, frames, loop, bgcolor, icc);

    if (!features.has_animation) {
        size_t bytes = (size_t)width * height * 4;
        uint8_t* rgba = malloc(bytes);
        if (!WebPDecodeRGBAInto(data, size, rgba, bytes, width * 4)) {
            printf("frame 0 error\n");
        } else {
            printf("frame 0 0 %016llx\n", (unsigned long long)fnv(rgba, bytes));
            write_frame(dir, file_index, 0, rgba, bytes);
        }
        free(rgba);
    } else {
        WebPDemuxer* demux = WebPDemux(&webp_data);
        size_t bytes = (size_t)anim_info.canvas_width * anim_info.canvas_height * 4;
        for (int index = 0; index < frames; ++index) {
            int duration = 0;
            WebPIterator iter;
            if (demux && WebPDemuxGetFrame(demux, index + 1, &iter)) {
                duration = iter.duration;
                WebPDemuxReleaseIterator(&iter);
            }
            uint8_t* canvas = NULL;
            int timestamp = 0;
            if (!WebPAnimDecoderGetNext(anim, &canvas, &timestamp)) {
                printf("frame %d error\n", index);
                break;
            }
            printf("frame %d %d %016llx\n", index, duration, (unsigned long long)fnv(canvas, bytes));
            write_frame(dir, file_index, index, canvas, bytes);
        }
        WebPDemuxDelete(demux);
        WebPAnimDecoderDelete(anim);
    }
    free(data);
}

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int bench(int count, const char* path) {
    size_t size = 0;
    uint8_t* data = read_file(path, &size);
    int width = 0, height = 0;
    if (!data || !WebPGetInfo(data, size, &width, &height)) return 1;
    size_t bytes = (size_t)width * height * 4;
    uint8_t* rgba = malloc(bytes);
    double best = 1e30;
    for (int i = 0; i < count; ++i) {
        double start = now_ms();
        if (!WebPDecodeRGBAInto(data, size, rgba, bytes, width * 4)) return 1;
        double took = now_ms() - start;
        if (took < best) best = took;
    }
    printf("%s %dx%d best %.3f ms\n", path, width, height, best);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 4 && !strcmp(argv[1], "--bench")) return bench(atoi(argv[2]), argv[3]);
    const char* dir = NULL;
    int first = 1;
    if (argc >= 3 && !strcmp(argv[1], "-o")) {
        dir = argv[2];
        first = 3;
    }
    for (int i = first; i < argc; ++i) run(argv[i], i - first, dir);
    return 0;
}
