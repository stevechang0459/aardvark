#include "checksum.h"

u8 checksum8_append(u8 chksum, const char *in, u32 len)
{
	while (len-- > 0) {
		chksum += *in++;
	}

	return chksum;
}

u8 checksum8(u8 chksum, const char *in, u32 len)
{
	chksum = checksum8_append(chksum, in, len);

	return (~chksum) + 1;
}
