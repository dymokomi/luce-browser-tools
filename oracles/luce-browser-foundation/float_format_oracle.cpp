// AK's `{}` of floats and doubles (Formatter<float>/<double> -> put_f32_or_f64 -> fmt's
// dragonbox), the oracle of luce-browser-foundation ak's float formatting tests.
//   sh float_format_build.sh && ./float_format_oracle < values.txt
// Each input line is `f32 BITS` or `f64 BITS` (hex bits of the value); each output line is
// `f32 BITS TEXT` with AK's formatting of that value.
#include <AK/Format.h>
#include <AK/String.h>
#include <stdio.h>
#include <string.h>

int main()
{
    char kind[8];
    unsigned long long bits;
    while (scanf("%7s %llx", kind, &bits) == 2) {
        if (strcmp(kind, "f32") == 0) {
            float value = bit_cast<float>(static_cast<u32>(bits));
            auto text = MUST(String::formatted("{}", value));
            printf("f32 %08llx %s\n", bits, text.to_byte_string().characters());
        } else {
            double value = bit_cast<double>(static_cast<u64>(bits));
            auto text = MUST(String::formatted("{}", value));
            printf("f64 %016llx %s\n", bits, text.to_byte_string().characters());
        }
    }
}
