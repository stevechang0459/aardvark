#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include "types.h"

void *aligned_alloc(size_t size, u32 align);
void aligned_free(void *aligned_ptr);

size_t strlen(const char *s);
void print_buf(const void *buf, size_t size, char *title);
void reverse(void *in, u32 len);

#define DBGPRINT(filter, ...) \
    printf(__VA_ARGS__)

#endif // ~ UTILITY_H
