#ifndef SMBUS_CORE_H
#define SMBUS_CORE_H

#include <stdbool.h>

#include "aardvark.h"
#include "types.h"

int smbus_send_byte(Aardvark handle, u8 tar_addr, u8 data, bool pec_flag);
int smbus_write_byte(Aardvark handle, u8 tar_addr, u8 cmd_code, u8 data,
                     bool pec_flag);
int smbus_write_word(Aardvark handle, u8 tar_addr, u8 cmd_code, u16 data,
                     bool pec_flag);
int smbus_write32(Aardvark handle, u8 tar_addr, u8 cmd_code, u32 data,
                  bool pec_flag);
int smbus_write64(Aardvark handle, u8 tar_addr, u8 cmd_code, u64 data,
                  bool pec_flag);
int smbus_write_file(Aardvark handle, u8 tar_addr, u8 cmd_code,
                     const char *file_name, bool pec_flag);
int smbus_block_write(Aardvark handle, u8 slave_addr, u8 cmd_code,
                      u8 byte_cnt, const void *buf, bool pec_flag);

int smbus_arp_cmd_prepare_to_arp(Aardvark handle, bool pec_flag);
int smbus_arp_cmd_reset_device(Aardvark handle, u8 tar_addr, u8 directed,
                               bool pec_flag);
int smbus_arp_cmd_get_udid(Aardvark handle, void *udid, u8 tar_addr,
                           bool directed, bool pec_flag);
int smbus_arp_cmd_assign_address(Aardvark handle, const void *udid, u8 tar_addr,
                                 bool pec_flag);

#endif // ~ SMBUS_CORE_H
