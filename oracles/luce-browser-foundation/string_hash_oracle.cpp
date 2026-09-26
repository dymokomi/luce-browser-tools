#include <AK/StringHash.h>
#include <stdio.h>
int main() {
    char const s[] = "\xff\x80\xc3\xa9z";
    printf("string_hash(ff 80 c3 a9 7a)=%u\n", AK::string_hash(s, 5));
    printf("string_hash(c3 a9)=%u\n", AK::string_hash(s + 2, 2));
    printf("ci_hash(ff 80 c3 a9 7a)=%u\n", AK::case_insensitive_string_hash(s, 5));
    char16_t const u[] = { 0xffff, 0x8000, 0xe9 };
    printf("u16=%u\n", AK::string_hash(u, 3));
    printf("char signed=%d\n", (int)(char)-1 < 0);
}
