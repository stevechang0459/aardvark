#ifndef SMBUS_CORE_H
#define SMBUS_CORE_H

#include <stdbool.h>

#include "aardvark.h"
#include "types.h"

#define BLOCK_SIZE_MAX                  (250)
#define PACKET_SIZE_MAX                 (4 + BLOCK_SIZE_MAX)
#define I2C_DEFAULT_BITRATE             100

// 0x61 << 1 = 0xC2
#define SMBUS_ADDR_DEFAULT              (0x61)

#define SMBUS_ARP_PREPARE_TO_ARP        (0x1)

int smbus_send_byte(Aardvark handle, u8 tar_addr, u8 data, u8 pec_flag);
int smbus_write_byte(Aardvark handle, u8 tar_addr, u8 cmd_code, u8 data,
                     u8 pec_flag);
int smbus_write_word(Aardvark handle, u8 tar_addr, u8 cmd_code, u16 data,
                     u8 pec_flag);
int smbus_write32(Aardvark handle, u8 tar_addr, u8 cmd_code, u32 data,
                  u8 pec_flag);
int smbus_write64(Aardvark handle, u8 tar_addr, u8 cmd_code, u64 data,
                  u8 pec_flag);
int smbus_write_file(Aardvark handle, u8 tar_addr, u8 cmd_code,
                     const char *file_name, u8 pec);
int smbus_write_block(Aardvark handle, u8 tar_addr, u8 cmd_code,
                      const u8 *block, u8 byte_count, u8 pec);

int smbus_prepare_to_arp(Aardvark handle, bool pec_flag);

#endif // ~ SMBUS_CORE_H