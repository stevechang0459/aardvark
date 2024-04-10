#ifndef CRC8_H
#define CRC8_H

#include "global.h"

#define CRC8_POLYNOMIAL (0x07)
// #define CRC8_POLYNOMIAL (0x31)

extern u8 crc8(u8 crc, const u8 *data, size_t count);
extern u8 crc8_mr(u8 crc, const u8 *data, size_t count);
extern u8 crc8_linux(u8 crc, const u8 *data, size_t count);

#endif