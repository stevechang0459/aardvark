#include "crc8.h"

u8 crc8(u8 crc, const u8 *data, size_t len)
{
	while (len--) {
		crc ^= *data++;
		for (int i = 8; i > 0; i--) {
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC8_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

u8 crc8_mr(u8 crc, const u8 *data, size_t count)
{
	u8 i = 0;

	if (count <= 0)
		return crc;

	while (count--) {
		for (i = 0x80; i != 0; i /= 2) {
			if ((crc & 0x80) != 0) {
				crc *= 2;
				crc ^= CRC8_POLYNOMIAL;
			} else {
				crc *= 2;
			}
			if ((*data & i) != 0) {
				crc ^= CRC8_POLYNOMIAL;
			}
		}
		data++;
	}

	return crc;
}

#define POLY    (0x1070U << 3)
static u8 _crc8_linux(u16 data)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (data & 0x8000)
			data = data ^ POLY;
		data = data << 1;
	}
	return (u8)(data >> 8);
}

u8 crc8_linux(u8 crc, const u8 *data, size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		crc = _crc8_linux((crc ^ data[i]) << 8);
	return crc;
}
