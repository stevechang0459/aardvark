#ifndef SMBUS_H
#define SMBUS_H

#include "types.h"

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