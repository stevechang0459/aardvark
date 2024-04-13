#ifndef SMBUS_CORE_H
#define SMBUS_CORE_H

#include "aardvark.h"
#include "types.h"

#define BLOCK_SIZE_MAX                  256
#define I2C_DEFAULT_BITRATE             100

extern int smbus_write_file(Aardvark handle, u8 tar_addr, u8 cmd_code,
                            const char *file_name, u8 pec);
extern int smbus_write_block(Aardvark handle, u8 tar_addr, u8 cmd_code,
                             const u8 *block, u8 len, u8 pec);

#endif // ~ SMBUS_CORE_H