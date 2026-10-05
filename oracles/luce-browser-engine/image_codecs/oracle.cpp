// Region p2img's oracle: what Ladybird's ImageDecoder process makes of an image file
// (Services/ImageDecoder/ConnectionFromClient.cpp decode_image_to_details over LibGfx's
// decoders): frame count, animated, loop count, size, and for each frame its duration and an
// FNV-1a hash of its pixels as premultiplied BGRA8888 (Skia's rounding, as the engine's plugin
// makes them), with a few pixels written out. Usage: oracle FILE...
#include <AK/Format.h>
#include <LibCore/MappedFile.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ImageFormats/ImageDecoder.h>

static u8 premultiply(u8 c, u8 a)
{
    u32 prod = u32(c) * u32(a) + 128;
    return u8((prod + (prod >> 8)) >> 8);
}

static u32 premultiplied_bgra(Gfx::Bitmap const& bitmap, int x, int y)
{
    auto color = bitmap.get_pixel(x, y);
    u8 a = color.alpha(), r = color.red(), g = color.green(), b = color.blue();
    if (bitmap.alpha_type() == Gfx::AlphaType::Unpremultiplied) {
        r = premultiply(r, a);
        g = premultiply(g, a);
        b = premultiply(b, a);
    }
    return (u32(a) << 24) | (u32(r) << 16) | (u32(g) << 8) | b;
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        StringView path { argv[i], strlen(argv[i]) };
        auto file = MUST(Core::MappedFile::map(path));
        auto decoder_or_error = Gfx::ImageDecoder::try_create_for_raw_bytes(file->bytes());
        if (decoder_or_error.is_error() || !decoder_or_error.value()) {
            outln("{}: no decoder", path);
            continue;
        }
        auto decoder = decoder_or_error.release_value();
        outln("{}: frames {} animated {} loop {} size {}x{}", path, decoder->frame_count(), decoder->is_animated(), decoder->loop_count(), decoder->width(), decoder->height());
        for (size_t index = 0; index < decoder->frame_count(); ++index) {
            auto frame_or_error = decoder->frame(index);
            if (frame_or_error.is_error()) {
                outln("  frame {}: error {}", index, frame_or_error.error());
                continue;
            }
            auto frame = frame_or_error.release_value();
            auto& bitmap = *frame.image;
            u32 hash = 2166136261u;
            StringBuilder pixels;
            for (int y = 0; y < bitmap.height(); ++y) {
                for (int x = 0; x < bitmap.width(); ++x) {
                    u32 pixel = premultiplied_bgra(bitmap, x, y);
                    for (int byte = 0; byte < 4; ++byte) {
                        hash ^= (pixel >> (8 * byte)) & 0xff;
                        hash *= 16777619u;
                    }
                    if (y * bitmap.width() + x < 4)
                        pixels.appendff(" {:08x}", pixel);
                }
            }
            outln("  frame {}: {}x{} duration {} hash {:08x} first{}", index, bitmap.width(), bitmap.height(), frame.duration, hash, pixels.string_view());
        }
    }
    return 0;
}
