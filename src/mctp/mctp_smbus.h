#ifndef MCTP_SMBUS_H
#define MCTP_SMBUS_H

#include "smbus.h"
#include "mctp.h"

#include "types.h"
#include <stdbool.h>

/**
 * DSP0236, Header version
 *
 * Identifies the format, physical framing, and data integrity mechanism used to
 * transfer the MCTP common fields in messages on a given physical medium. The
 * value is defined in the specifications for the particular medium.
 *
 * Note: The value in this field can vary between different transport bindings.
 *
 * ---
 * DSP0237, MCTP header version
 *
 * Set to 0001b for MCTP devices that are conformant to the MCTP Base
 * Specification 1.0 and this version of the SMBus transport binding.
 */
#define MCTP_HEADER_VERSION             (1)

#define MCTP_TRAN_UNIT_SIZE_MAX         (250)
#define MCTP_SMBUS_PACKET_SIZE          (sizeof(union mctp_smbus_header) + sizeof(u32) + \
                                        MCTP_TRAN_UNIT_SIZE_MAX + OPT_SMBUS_PEC_SUPPORT)

/**
 * DSP0237
 *
 * The value enables MCTP to be differentiated from IPMI over SMBus and IPMB
 * (IPMI over I2C) protocols.
 */
enum transport_binding {
	// IPMI over SMBus and IPMB (IPMI over I2C) protocol
	IPMI_OVER_SMBUS = 0,
	// MCTP over SMBus
	MCTP_OVER_SMBUS = 1
};

union mctp_smbus_header {
	struct {
		u8 dev_slv_addr;
		u8 cmd_code;
		u8 byte_cnt;
		u8 src_slv_addr;
	} __attribute__((packed));
	u32 value;
} __attribute__((packed));

union mctp_smbus_packet {
	struct {
		union mctp_smbus_header medi_head;
		union mctp_transport_header tran_head;
		u8 payload[MCTP_TRAN_UNIT_SIZE_MAX];
#if (OPT_SMBUS_PEC_SUPPORT)
		u8 pec;
#endif
	} __attribute__((packed));
	u8 data[MCTP_SMBUS_PACKET_SIZE];
} __attribute__((packed));

struct mctp_smbus_context {
	int handle;
	u8 src_slv_addr;
	bool pec_enabled;
};

int mctp_smbus_check_packet(const union mctp_smbus_header *medi_head);
int mctp_smbus_transmit_packet(u8 dst_slv_addr, union mctp_smbus_packet *pkt,
                               u8 tran_size, int verbose);
int mctp_smbus_init(int handle, u8 src_slv_addr, bool pec_flag);
int mctp_smbus_deinit(void);

#endif // ~ MCTP_SMBUS_H
