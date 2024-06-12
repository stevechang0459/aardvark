#ifndef UTILITY_H
#define UTILITY_H

#include "smbus.h"

#include "types.h"
#include <stdbool.h>

#include <stddef.h>

void *aligned_alloc(size_t size, u32 align);
void aligned_free(void *aligned_ptr);

size_t strlen(const char *s);
void print_buf(const void *buf, size_t size, const char *title, ...);
void reverse(void *in, u32 len);
void print_udid(const union udid_ds *udid);

#define DBGPRINT(filter, ...) \
    printf(__VA_ARGS__)

#endif // ~ UTILITY_H
