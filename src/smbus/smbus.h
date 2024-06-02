#ifndef SMBUS_H
#define SMBUS_H

#include "types.h"

//
#define OPT_SMBUS_PEC_SUPPORT           (1)

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

enum smbus_command_code {
	// SMBus block read of the drive’s status
	SMBUS_CMD_CODE_BRDS = 0x00,
	// SMBus block read of the drive’s static data
	SMBUS_CMD_CODE_BRDD = 0x08,
	//
	SMBUS_CMD_CODE_MCTP = 0x0F,
	// SMBus send byte to reset Arbitration bit
	SMBUS_CMD_CODE_RAB = 0xFF
};

enum smbus_cmd_status {
	SMBUS_CMD_SUCCESS = 0,
	SMBUS_CMD_ERROR = 1,
	SMBUS_CMD_WRITE_FAILED,
	SMBUS_CMD_READ_FAILED,
	SMBUS_CMD_NUM_WRITTEN_MISMATCH,
	SMBUS_CMD_NUM_READ_MISMATCH,
	SMBUS_CMD_BYTE_CNT_ERR,
	SMBUS_CMD_DEV_TAR_ADDR_ERR,
	SMBUS_CMD_PEC_ERR,
};

union smbus_prepare_to_arp_ds {
	struct {
		u8 slave_addr;
		u8 cmd;
		u8 pec;
	};
	u8 data[3];
} __attribute__((packed));

union smbus_get_udid_ds {
	struct {
		u8 slave_addr1;
		u8 cmd;
		u8 slave_addr2;
		u8 byte_cnt;
		u8 udid[16];
		// Device Target Address
		u8 dev_tar_addr;
		u8 pec;
	};
	u8 data[22];
} __attribute__((packed));

union smbus_reset_device_ds {
	struct {
		u8 slave_addr;
		u8 cmd;
		u8 pec;
	};
	u8 data[3];
} __attribute__((packed));

union smbus_assign_address_ds {
	struct {
		u8 slave_addr1;
		u8 cmd;
		u8 slave_addr2;
		u8 byte_cnt;
		u8 udid[16];
		// Device Target Address
		u8 dev_tar_addr;
		u8 pec;
	};
	u8 data[22];
} __attribute__((packed));

#endif // ~ SMBUS_H
