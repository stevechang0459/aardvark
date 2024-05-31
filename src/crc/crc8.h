#ifndef CRC8_H
#define CRC8_H

#include <stddef.h>
#include "types.h"

#define POLY_CRC8                                               (0x07)

u8 crc8(u8 crc, const void *buf, size_t count);
u8 crc8_mr(u8 crc, const void *buf, size_t count);
u8 crc8_linux(u8 crc, const void *buf, size_t count);

#endif