#ifndef SMBUS_H
#define SMBUS_H

#include <stdbool.h>

#include "aardvark.h"
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
	} __attribute__((packed));
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
	} __attribute__((packed));
	u8 data[22];
} __attribute__((packed));

union smbus_reset_device_ds {
	struct {
		u8 slave_addr;
		u8 cmd;
		u8 pec;
	} __attribute__((packed));
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
	} __attribute__((packed));
	u8 data[22];
} __attribute__((packed));

union udid_device_capabilities {
	struct {
		// PEC Supported
		u8 pec_sup : 1;
		// Reserved
		u8 rsvd : 5;
		// Address Type
		u8 addr_type : 2;
	};
	u8 value;
} __attribute__((packed));

union udid_version_revision {
	struct {
		// Silicon Revision ID
		u8 si_rev_id : 3;
		// UDID Version
		u8 udid_ver : 3;
		// Reserved
		u8 rsvd : 2;
	};
	u8 value;
} __attribute__((packed));

union udid_interface {
	struct {
		struct {
			u8 smbus_ver : 4;
			u8 oem : 1;
			u8 asf : 1;
			u8 ipmi : 1;
			u8 zone : 1;
		};
		u8 rsvd;
	} __attribute__((packed));;
	u16 value;
} __attribute__((packed));

union udid_ds {
	struct {
		u32 vendor_spec_id;
		u16 subsys_device_id;
		u16 subsys_vendor_id;
		// Interface
		union udid_interface interface;
		// Device ID
		u16 device_id;
		// Vendor ID
		u16 vendor_id;
		// Version/Revision
		union udid_version_revision ver_rev;
		// Device Capabilities
		union udid_device_capabilities dev_cap;
	} __attribute__((packed));
	u8 data[16];
} __attribute__((packed));


int smbus_send_byte(Aardvark handle, u8 slv_addr, u8 data, bool pec_flag);
int smbus_write_byte(Aardvark handle, u8 slv_addr, u8 cmd_code, u8 data,
                     bool pec_flag);
int smbus_write_word(Aardvark handle, u8 slv_addr, u8 cmd_code, u16 data,
                     bool pec_flag);
int smbus_write32(Aardvark handle, u8 slv_addr, u8 cmd_code, u32 data,
                  bool pec_flag);
int smbus_write64(Aardvark handle, u8 slv_addr, u8 cmd_code, u64 data,
                  bool pec_flag);
int smbus_write_file(Aardvark handle, u8 slv_addr, u8 cmd_code,
                     const char *file_name, bool pec_flag);
int smbus_block_write(Aardvark handle, u8 slave_addr, u8 cmd_code,
                      u8 byte_cnt, const void *buf, bool pec_flag);

int smbus_arp_cmd_prepare_to_arp(Aardvark handle, bool pec_flag);
int smbus_arp_cmd_reset_device(Aardvark handle, u8 slv_addr, u8 directed,
                               bool pec_flag);
int smbus_arp_cmd_get_udid(Aardvark handle, void *udid, u8 slv_addr,
                           bool directed, bool pec_flag);
int smbus_arp_cmd_assign_address(Aardvark handle, const void *udid, u8 slv_addr,
                                 bool pec_flag);

#endif // ~ SMBUS_H
