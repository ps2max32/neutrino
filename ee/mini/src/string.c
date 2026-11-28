// Part of neutrino: the EE side without an SDK. See README.md.
// Built with -fno-builtin: the loops are not turned back into calls
#include <string.h>

#include <tamtypes.h>

void *memcpy(void *dst, const void *src, size_t n)
{
    u8 *d       = dst;
    const u8 *s = src;

    if ((((u32)d | (u32)s) & 15) == 0) {
        for (; n >= 16; n -= 16, d += 16, s += 16)
            *(u128 *)d = *(const u128 *)s;
    } else if ((((u32)d | (u32)s) & 3) == 0) {
        for (; n >= 4; n -= 4, d += 4, s += 4)
            *(u32 *)d = *(const u32 *)s;
    }
    while (n--)
        *d++ = *s++;

    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    u8 *d = dst;
    u32 v = (u8)c * 0x01010101u;

    if (((u32)d & 3) == 0)
        for (; n >= 4; n -= 4, d += 4)
            *(u32 *)d = v;
    while (n--)
        *d++ = c;

    return dst;
}

size_t strlen(const char *s)
{
    size_t n = 0;

    while (s[n])
        n++;

    return n;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = 0;

    return dst;
}
