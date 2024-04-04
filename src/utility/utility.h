#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"
#include <stdbool.h>

#include <stddef.h>

#ifdef WIN32
void *align_alloc(size_t size, u32 align);
void align_free(void *aligned_ptr);
#endif
size_t strlen(const char *s);
void print_buf(const void *buf, size_t size, const char *title, ...);
void reverse(void *in, u32 len);

#define DBGPRINT(filter, ...)   printf(__VA_ARGS__)

#endif // ~ UTILITY_H
