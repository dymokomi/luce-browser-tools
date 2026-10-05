// Oracle for raster's ft_grays: FreeType 2.13.3's FT_Outline_Get_Bitmap (the vcpkg build Ladybird uses).
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <stdio.h>
#include <string.h>

static FT_Library lib;

// pts: x,y pairs in 26.6; tags; ends.
static void render(const char* name, int n, const long* pts, const char* tags, int nc, const short* ends, int even_odd, int w, int h)
{
    FT_Vector v[256]; char t[256]; short e[32];
    for (int i = 0; i < n; i++) { v[i].x = pts[2*i]; v[i].y = pts[2*i+1]; t[i] = tags[i]; }
    for (int i = 0; i < nc; i++) e[i] = ends[i];
    FT_Outline o; memset(&o, 0, sizeof o);
    o.n_points = n; o.n_contours = nc; o.points = v; o.tags = t; o.contours = e;
    o.flags = even_odd ? FT_OUTLINE_EVEN_ODD_FILL : 0;
    unsigned char buf[64*64]; memset(buf, 0, sizeof buf);
    FT_Bitmap b; memset(&b, 0, sizeof b);
    b.width = w; b.rows = h; b.pitch = w; b.buffer = buf; b.pixel_mode = FT_PIXEL_MODE_GRAY; b.num_grays = 256;
    int err = FT_Outline_Get_Bitmap(lib, &o, &b);
    printf("case %s %d %d %d\n", name, w, h, err);
    for (int y = 0; y < h; y++) { for (int x = 0; x < w; x++) printf("%02x", buf[y*w+x]); printf("\n"); }
}

int main()
{
    FT_Init_FreeType(&lib);
    // A five-pointed star (self-overlapping), lines only, at fractional 26.6 positions.
    { long p[] = { 640,1250, 830,60, 130,800, 1150,820, 450,70 }; char t[] = {1,1,1,1,1}; short e[] = {4};
      render("star_nonzero", 5, p, t, 1, e, 0, 20, 20); render("star_even_odd", 5, p, t, 1, e, 1, 20, 20); }
    // TrueType quadratics: consecutive off points (implied midpoints), a contour starting on an
    // off point whose last point is on, and one whose first and last points are both off.
    { long p[] = { 100,600, 100,1100, 600,1100, 1100,1100, 1100,600, 1100,100, 600,100, 100,100,
                   300,600, 600,900, 900,600, 600,300 };
      char t[] = {1,0,1,0,1,0,1,0, 0,0,0,0}; short e[] = {7, 11};
      render("conics", 12, p, t, 2, e, 0, 20, 20); }
    { long p[] = { 200,200, 1000,250, 1100,1000, 300,1150, 150,700 };
      char t[] = {0,1,0,1,1}; short e[] = {4};
      render("conic_start_off", 5, p, t, 1, e, 0, 20, 20); }
    // Cubics (CFF style), one contour wound the other way inside.
    { long p[] = { 64,640, 64,1300, 1216,1300, 1216,640, 1216,-20, 64,-20,
                   400,640, 400,300, 880,300, 880,640, 880,980, 400,980 };
      char t[] = {1,2,2,1,2,2, 1,2,2,1,2,2}; short e[] = {5, 11};
      render("cubics", 12, p, t, 2, e, 0, 22, 22); }
    // Clipped by the bitmap on every side, with steep and shallow edges.
    { long p[] = { -300,500, 700,-250, 1500,600, 900,1400, 20,1350 }; char t[] = {1,1,1,1,1}; short e[] = {4};
      render("clipped", 5, p, t, 1, e, 0, 16, 16); }
    // A thin sliver and a large curve (several conic subdivisions).
    { long p[] = { 70,70, 2500,90, 2500,110, 70,100,  200,300, 2400,3800, 3000,300 };
      char t[] = {1,1,1,1, 1,0,1}; short e[] = {3, 6};
      render("sliver_and_big_conic", 7, p, t, 2, e, 0, 48, 64); }
    return 0;
}
