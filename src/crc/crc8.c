#include "crc8.h"
#include "types.h"

u8 crc8(const void *buf, size_t len)
{
	const u8 *data = buf;
	u8 crc = INIT_CRC8;

	while (len--) {
		crc ^= *data++;
		for (int i = 8; i > 0; i--) {
			if (crc & 0x80)
				crc = (crc << 1) ^ POLY_CRC8;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

u8 crc8_mr(u8 crc, const void *buf, size_t len)
{
	const u8 *data = buf;
	u8 i = 0;
	if (len <= 0)
		return crc;

	while (len--) {
		for (i = 0x80; i != 0; i /= 2) {
			if ((crc & 0x80) != 0) {
				crc *= 2;
				crc ^= POLY_CRC8;
			} else {
				crc *= 2;
			}
			if ((*data & i) != 0) {
				crc ^= POLY_CRC8;
			}
		}
		data++;
	}

	return crc;
}

#define POLY_CRC32    (0x1070U << 3)
static u8 _crc8_linux(u16 data)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (data & 0x8000)
			data = data ^ POLY_CRC32;
		data = data << 1;
	}
	return (u8)(data >> 8);
}

u8 crc8_linux(u8 crc, const void *buf, size_t len)
{
	const u8 *data = buf;
	int i;

	for (i = 0; i < len; i++)
		crc = _crc8_linux((crc ^ data[i]) << 8);
	return crc;
}
