#ifndef CRC8_H
#define CRC8_H

#include <stddef.h>
#include "types.h"

#define CRC8_INIT                                               (0)
#define CRC8_POLY                       (0x07)

u8 crc8(const void *buf, size_t len);
u8 crc8_mr(u8 crc, const void *buf, size_t len);
u8 crc8_linux(u8 crc, const void *buf, size_t len);

#endif