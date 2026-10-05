// What Ladybird's ImageDecoder (PNGLoader with Skia's compositing) makes of an APNG, frame by
// frame, as straight RGBA bytes: writes frame N of FILE to OUTDIR/N.rgba. For comparing
// luce-png's OVER compositing with Skia's. Usage: ladybird_frames FILE OUTDIR
#include <AK/Format.h>
#include <LibCore/MappedFile.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ImageFormats/ImageDecoder.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc < 3) return 2;
    auto file = MUST(Core::MappedFile::map(StringView { argv[1], strlen(argv[1]) }));
    auto decoder = MUST(Gfx::ImageDecoder::try_create_for_raw_bytes(file->bytes()));
    for (size_t index = 0; index < decoder->frame_count(); ++index) {
        auto frame = MUST(decoder->frame(index));
        auto& bitmap = *frame.image;
        char path[4096];
        snprintf(path, sizeof path, "%s/%zu.rgba", argv[2], index);
        FILE* out = fopen(path, "wb");
        for (int y = 0; y < bitmap.height(); ++y) {
            for (int x = 0; x < bitmap.width(); ++x) {
                auto c = bitmap.get_pixel(x, y);
                unsigned char px[4] = { c.red(), c.green(), c.blue(), c.alpha() };
                fwrite(px, 1, 4, out);
            }
        }
        fclose(out);
    }
    return 0;
}
