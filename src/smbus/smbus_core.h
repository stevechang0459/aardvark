#ifndef SMBUS_CORE_H
#define SMBUS_CORE_H

#include <stdbool.h>

#include "aardvark.h"
#include "types.h"

#define BLOCK_SIZE_MAX                  (250)
#define SMBUS_BUF_MAX                   (259)
#define I2C_DEFAULT_BITRATE             100

// 1100_001xb, 0xC2, SMBus Device Default Address
#define SMBUS_ADDR_DEFAULT              (0x61)
// 1101_010xb, 0xD4, Basic Management Command
#define SMBUS_ADDR_NVME_MI_BMC          (0x6A)

enum i2c_data_direction {
	I2C_WRITE = 0,
	I2C_READ = 1,
};

enum smbus_arp_command {
	// Prepare to ARP
	SMBUS_ARP_PREPARE_TO_ARP = 1,
	// Reset device
	SMBUS_ARP_RESET_DEVICE = 2,
	// Get UDID
	SMBUS_ARP_GET_UDID,
	// Assign address
	SMBUS_ARP_ASSIGN_ADDRESS,
};

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
int smbus_block_write(Aardvark handle, u8 slave_addr, u8 cmd_code,
                      u8 byte_cnt, const void *buf, u8 pec_flag);

int smbus_arp_cmd_prepare_to_arp(Aardvark handle, bool pec_flag);
int smbus_arp_cmd_reset_device(Aardvark handle, u8 tar_addr, u8 directed,
                               bool pec_flag);
int smbus_arp_cmd_get_udid(Aardvark handle, void *udid, u8 tar_addr,
                           bool directed, bool pec_flag);
int smbus_arp_cmd_assign_address(Aardvark handle, const void *udid, u8 tar_addr,
                                 bool pec_flag);

#endif // ~ SMBUS_CORE_H