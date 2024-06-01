#ifndef SMBUS_H
#define SMBUS_H

#include "types.h"

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

union smbus_assign_addr_ds {
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

#endif