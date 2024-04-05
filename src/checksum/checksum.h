#ifndef CHECKSUM_H
#define CHECKSUM_H

#include "types.h"

u8 checksum8_append(u8 chksum, const char *in, u32 len);
u8 checksum8(u8 chksum, const char *in, u32 len);

#endif
