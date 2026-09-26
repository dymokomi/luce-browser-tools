/* Driver printing libregexp results for the luce_js.regexp tests. Reads lines:
   C <mode> <flags> <pattern hex>
   X <mode> <flags> <pattern hex> <cbuf_type> <cindex> <subject units hex, 4 digits each or 2 for type 0>
   Prints one line per request. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cutils.h"
#include "libregexp.h"

static int g_mode;
static int g_alloc_left; /* for mode >= 100 */

int lre_check_stack_overflow(void *opaque, size_t alloca_size) { return g_mode == 2; }
int lre_check_timeout(void *opaque) { return g_mode == 1; }
void *lre_realloc(void *opaque, void *ptr, size_t size)
{
    if (size == 0) { free(ptr); return NULL; }
    if (g_mode >= 100) {
        if (g_alloc_left <= 0) return NULL;
        g_alloc_left--;
    }
    return realloc(ptr, size);
}

static int unhex(const char *h, uint8_t *out)
{
    int n = 0;
    while (h[0] && h[1] && h[0] != ' ' && h[0] != '\n') {
        unsigned v; sscanf(h, "%2x", &v); out[n++] = v; h += 2;
    }
    out[n] = 0;
    return n;
}

static uint32_t fnv(const uint8_t *p, int n)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

/* the compile description */
static void describe(uint8_t *bc, int len, const char *err)
{
    if (!bc) { printf("err=%s\n", err); return; }
    printf("ok len=%d cc=%d alloc=%d flags=0x%x", len, lre_get_capture_count(bc),
           lre_get_alloc_count(bc), lre_get_flags(bc));
    const char *names = lre_get_groupnames(bc);
    if (names) {
        printf(" names=");
        for (int i = 1; i < lre_get_capture_count(bc); i++) {
            if (i != 1) printf(",");
            printf("<%s>", names);
            names += strlen(names) + LRE_GROUP_NAME_TRAILER_LEN;
        }
    }
    if (len <= 200) {
        printf(" bc=");
        for (int i = 0; i < len; i++) printf("%02x", bc[i]);
    } else {
        printf(" hash=%08x", fnv(bc, len));
    }
    printf("\n");
}

int main(void)
{
    static char line[1 << 16];
    static uint8_t pat[1 << 15], subj[1 << 15];
    while (fgets(line, sizeof(line), stdin)) {
        char kind; int mode, flags;
        char *p = line;
        kind = *p; p += 2;
        if (kind == 'E') {
            int allow = strtol(p, &p, 10); p++;
            unhex(p, pat);
            const uint8_t *q = pat;
            int ret = lre_parse_escape(&q, allow);
            printf("%d %d\n", ret, (int)(q - pat));
            fflush(stdout);
            continue;
        }
        mode = strtol(p, &p, 10); p++;
        flags = strtol(p, &p, 10); p++;
        int plen = unhex(p, pat);
        p = strchr(p, ' ');
        char err[64];
        int len;
        g_mode = (kind == 'C') ? mode : 0;
        g_alloc_left = mode - 100;
        uint8_t *bc = lre_compile(&len, err, sizeof(err), (char *)pat, plen, flags, NULL);
        if (kind == 'C') {
            describe(bc, len, err);
        } else {
            int cbuf_type, cindex;
            p++;
            cbuf_type = strtol(p, &p, 10); p++;
            cindex = strtol(p, &p, 10);
            int n = 0;
            if (*p == ' ') {
                p++;
                n = unhex(p, subj);
            }
            int clen;
            uint16_t subj16[1 << 14];
            const uint8_t *cbuf;
            if (cbuf_type == 0) { clen = n; cbuf = subj; }
            else {
                clen = n / 2;
                for (int i = 0; i < clen; i++) subj16[i] = (subj[2*i] << 8) | subj[2*i+1];
                cbuf = (uint8_t *)subj16;
            }
            if (!bc) { printf("compile error %s\n", err); continue; }
            uint8_t **capture = malloc(sizeof(capture[0]) * (lre_get_alloc_count(bc) + 1));
            g_mode = mode;
            g_alloc_left = mode - 100;
            int ret = lre_exec(capture, bc, cbuf, cindex, clen, cbuf_type, NULL);
            printf("%d", ret);
            if (ret == 1) {
                int cc = lre_get_capture_count(bc);
                for (int i = 0; i < cc; i++) {
                    if (!capture[2*i] || !capture[2*i+1]) printf(" -");
                    else printf(" %d,%d", (int)((capture[2*i] - cbuf) >> cbuf_type),
                                (int)((capture[2*i+1] - cbuf) >> cbuf_type));
                }
            }
            printf("\n");
            free(capture);
        }
        free(bc);
        fflush(stdout);
    }
    return 0;
}
