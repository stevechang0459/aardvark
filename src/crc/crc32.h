#ifndef CRC32_H
#define CRC32_H

#include "types.h"
#include <stddef.h>

// CRC-32C (Castagnoli)
#define POLY                            (0x1EDC6F41L)
/* CRC32C polynomial in reversed bit order. */
#define REVERSED_POLY                   (0x82F63b78L)

#define CRC_INIT                        (0xffffffffL)
#define XO_ROT                          (0xffffffffL)

u32 crc32(const void *buf, size_t len);
u32 crc32_halfbyte(const void *buf, size_t len);
u32 crc32_tableless(const void *buf, size_t len);

#endif // ~ CRC32_H
