// libjpeg_oracle: the ICC profile libjpeg-turbo's jpeg_read_icc_profile joins from a JPEG,
// read as Ladybird's JPEGLoader reads it (APP2 markers saved, every scanline read, then the
// profile), in the format of luce-jpeg's tests/icc_tool.lucb:
//   NAME icc=LENGTH:FNV1A | NAME icc=- | NAME error
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>

struct handler {
    struct jpeg_error_mgr pub;
    jmp_buf jump;
};

static void fail(j_common_ptr cinfo) { longjmp(((struct handler*)cinfo->err)->jump, 1); }
static void quiet(j_common_ptr cinfo, int level) { (void)cinfo; (void)level; }

static uint32_t fnv(const uint8_t* d, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 16777619u; }
    return h;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        FILE* f = fopen(argv[i], "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* data = malloc(size);
        fread(data, 1, size, f);
        fclose(f);
        const char* name = strrchr(argv[i], '/');
        printf("%s", name ? name + 1 : argv[i]);
        struct jpeg_decompress_struct cinfo;
        struct handler err;
        cinfo.err = jpeg_std_error(&err.pub);
        err.pub.error_exit = fail;
        err.pub.emit_message = quiet;
        unsigned char* row = NULL;
        if (setjmp(err.jump)) {
            printf(" error\n");
            jpeg_destroy_decompress(&cinfo);
            free(data);
            free(row);
            continue;
        }
        jpeg_create_decompress(&cinfo);
        jpeg_mem_src(&cinfo, data, size);
        jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xFFFF);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);
        row = malloc(cinfo.output_width * cinfo.output_components);
        while (cinfo.output_scanline < cinfo.output_height)
            if (jpeg_read_scanlines(&cinfo, &row, 1) == 0)
                break;
        JOCTET* icc = NULL;
        unsigned int length = 0;
        if (jpeg_read_icc_profile(&cinfo, &icc, &length)) {
            printf(" icc=%u:%08x\n", length, fnv(icc, length));
            free(icc);
        } else {
            printf(" icc=-\n");
        }
        jpeg_abort_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        free(row);
        free(data);
    }
    return 0;
}
