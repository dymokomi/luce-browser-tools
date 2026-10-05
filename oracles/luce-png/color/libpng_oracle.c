// libpng_oracle: the colour chunks libpng 1.6.50 (Ladybird's, from its vcpkg tree) reads from
// PNGs, in the format of luce-png's tests/color_tool.lucb:
//   NAME cicp=P,T,M,F|- icc=LENGTH:FNV1A|- srgb=I|- gama=G|- chrm=W...|-
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void quiet(png_structp p, png_const_charp m) { (void)p; (void)m; }

static uint32_t fnv(const uint8_t* d, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 16777619u; }
    return h;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        FILE* f = fopen(argv[i], "rb");
        if (!f) continue;
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, quiet, quiet);
        png_infop info = png_create_info_struct(png);
        const char* name = strrchr(argv[i], '/');
        printf("%s", name ? name + 1 : argv[i]);
        if (setjmp(png_jmpbuf(png))) {
            printf(" error\n");
            png_destroy_read_struct(&png, &info, NULL);
            fclose(f);
            continue;
        }
        png_init_io(png, f);
        png_read_info(png, info);
        png_byte cp, tc, mc, fr;
        if (png_get_cICP(png, info, &cp, &tc, &mc, &fr)) printf(" cicp=%u,%u,%u,%u", cp, tc, mc, fr); else printf(" cicp=-");
        png_charp pname; int comp; png_bytep prof; png_uint_32 plen;
        if (png_get_iCCP(png, info, &pname, &comp, &prof, &plen)) printf(" icc=%u:%08x", plen, fnv(prof, plen)); else printf(" icc=-");
        int intent;
        if (png_get_sRGB(png, info, &intent)) printf(" srgb=%d", intent); else printf(" srgb=-");
        png_fixed_point g;
        if (png_get_gAMA_fixed(png, info, &g)) printf(" gama=%d", (int)g); else printf(" gama=-");
        png_fixed_point wx, wy, rx, ry, gx, gy, bx, by;
        if (png_get_cHRM_fixed(png, info, &wx, &wy, &rx, &ry, &gx, &gy, &bx, &by)) printf(" chrm=%d,%d,%d,%d,%d,%d,%d,%d", wx, wy, rx, ry, gx, gy, bx, by); else printf(" chrm=-");
        printf("\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(f);
    }
    return 0;
}
