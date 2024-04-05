#ifndef UTILITY_H
#define UTILITY_H

#include "global.h"

void *aligned_alloc(size_t size, u32 align);
void aligned_free(void *aligned_ptr);

size_t strlen(const char *s);
void print_buf(const void *buf, size_t size, char *title);

#define DBGPRINT(filter, ...) \
    printf(__VA_ARGS__)

#endif // ~ UTILITY_H
