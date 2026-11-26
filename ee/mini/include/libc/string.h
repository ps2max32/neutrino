// Part of neutrino: the EE side without an SDK. See README.md.
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
size_t strlen(const char *s);
char *strncpy(char *dst, const char *src, size_t n);

#endif
