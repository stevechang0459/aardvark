#include <stdint.h>
#include "types.h"

u32 crc32(const char *data, int length)
{
#include "crctable.out"
	const uint32_t CRC_INIT = 0xffffffffL;
	const uint32_t XO_ROT   = 0xffffffffL;
	uint32_t crc = CRC_INIT;

	while (length--) {
		crc = crctable[(crc ^ *data++) & 0xFFL] ^ (crc >> 8);
	}
	crc = crc ^ XO_ROT;

	// printf("Input: %s\n", data);
	// printf("CRC: 0x%08x\n", crc);

	return crc;
}
