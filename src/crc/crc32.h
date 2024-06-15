#ifndef CRC32_H
#define CRC32_H

#include "types.h"
#include <stddef.h>

#define CRC_LE_BITS                     (8)

// CRC-32C (Castagnoli)
#define POLY_CRC32                      (0x1EDC6F41L)
/* CRC32C polynomial in reversed bit order. */
#define REVERSED_POLY_CRC32             (0x82F63b78L)

#define CRC_INIT                        (0xffffffffL)
#define XO_ROT                          (0xffffffffL)

u32 crc32_le_generic(u32 crc, const void *buf, size_t len, u32 poly);

#endif // ~ CRC32_H
