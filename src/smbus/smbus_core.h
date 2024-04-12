#ifndef SMBUS_CORE_H
#define SMBUS_CORE_H

#include "aardvark.h"
#include "types.h"

#define BUFFER_SIZE                     2048
#define I2C_DEFAULT_BITRATE             100

extern int smbus_write_file(Aardvark handle, u8 tar_addr, const char *filen_ame,
                            u8 pec);

#endif // ~ SMBUS_CORE_H