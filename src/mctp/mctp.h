#ifndef MCTP_H
#define MCTP_H

#include "types.h"
#include <stdbool.h>

#include <stdio.h>

//
#define MCTP_MSG_SIZE_MAX               (4224)
/**
 * DSP0236, 8.3 Packet payload and transmission unit sizes
 *
 * For MCTP, the size of a transmission unit is defined as the size of the
 * packet payload that is carried in an MCTP packet.
 *
 * The baseline transmission unit (minimum transmission unit) size for MCTP is
 * 64 bytes.
 */
/**
 * NVMe-MI, 3.2.1 MCTP Packet
 *
 * In the MCTP Base Specification, the smallest unit of data transfer is the
 * MCTP packet. One or more packets are combined to create an MCTP message. In
 * this specification, the MCTP messages are referred to as NVMe-MI Messages
 * (refer to section 1.8.21). Refer to section 3.2.1.1 for details on how MCTP
 * packets are assembled into NVMe-MI Messages. An MCTP Packet Payload contains
 * at least 1 byte but shall not exceed the negotiated MCTP Transmission Unit
 * Size. The format of an MCTP Packet Payload is shown in Figure 22.
 */
#define MCTP_BASELINE_TRAN_UNIT_SIZE    (64)

#define MCTP_TRACE_FILTER (0 \
        | BITLSHIFT(1, ERROR) \
        | BITLSHIFT(1, WARN) \
        | BITLSHIFT(1, DEBUG) \
        | BITLSHIFT(1, INFO) \
        | BITLSHIFT(1, INIT) \
        )

#if 1
extern const char *mctp_trace_header[];

#define mctp_trace(type, ...) \
do { \
        if (BITLSHIFT(1, type) & MCTP_TRACE_FILTER) { \
        fprintf(stderr, "%s", mctp_trace_header[type]); \
        fprintf(stderr, __VA_ARGS__);} \
} while (0)
#else
#define mctp_trace(type, ...) { \
if (BITLSHIFT(1, type) & MCTP_TRACE_FILTER) \
        fprintf(stderr, "[%s]: ", #type); \
        fprintf(stderr, __VA_ARGS__);}
#endif

enum special_eid {
	EID_NULL_DST = 0,
	EID_NULL_SRC = 0,
	EID_BROADCAST = 0xFF,
};

enum mctp_msg_type {
	MCTP_MSG_TYPE_CTRL = 0,
	MCTP_MSG_TYPE_PLDM = 1,
	MCTP_MSG_TYPE_NCSI,
	MCTP_MSG_TYPE_ETHERNET,
	MCTP_MSG_TYPE_NVME_MM,
	MCTP_MSG_TYPE_SPDM,
	MCTP_MSG_TYPE_SECURED,
	MCTP_MSG_TYPE_CXL_FX_API,
	MCTP_MSG_TYPE_CXL_CCI,
	MCTP_MSG_TYPE_CXL_VENDOR_PCI = 0x7E,
	MCTP_MSG_TYPE_CXL_VENDOR_IANA = 0x7F,
};

enum mctp_ctrl_cmd_code {
	MCTP_CTRL_MSG_SET_EID = 1,
	MCTP_CTRL_MSG_GET_EID = 2,
};

enum mctp_error_status {
	MCTP_SUCCESS = 0,
	MCTP_SMBUS_BUSY = 1,
	MCTP_TRAN_BUSY,
	MCTP_TRAN_SUCCESS,

	MCTP_ERROR,
	MCTP_SMBUS_ERR_UNSUP_TRAN_UNIT,
	MCTP_SMBUS_ERR_INVLD_SUP,
// Unexpected "middle" packet or "end" packet
	MCTP_TRAN_ERR_UNEXP_MID_PKT,
// Bad packet data integrity or other physical layer error
	MCTP_TRAN_ERR_DATA_INTEGRITY,
// Bad, unexpected, or expired message tag
	MCTP_TRAN_ERR_BAD_MSG_TAG,
// Unknown destination EID
	MCTP_TRAN_ERR_UNKNO_DST_EID,
// Un-routable EID
	MCTP_TRAN_ERR_UNROU_EID,
// Bad header version
	MCTP_TRAN_ERR_BAD_HEADER,
// Unsupported transmission unit
	MCTP_TRAN_ERR_UNSUP_TRAN_UNIT,
	MCTP_TRAN_ERR_UNEXP_MID_PKT2,
	MCTP_TRAN_ERR_BAD_MSG_TAG2,
// Out-of-sequence packet sequence number
	MCTP_TRAN_ERR_OOS_PKT_SEQ,
// Incorrect transmission unit
	MCTP_TRAN_ERR_INCORRECT_TRAN_UNIT,
// ---
	MCTP_TRAN_ERR_UNSUP_MSG_SIZE,
	MCTP_TRAN_ERR_UNKNO_DST_ADDR,
	MCTP_TRAN_ERR_TRAN_PKT_TIMEOUT,

	MCTP_MSG_ERR_CTRL_REQ_MSG,
	MCTP_MSG_ERR_CTRL_RESP_MSG,
};

enum set_eid_operation {
	SET_EID = 0,
	FORCE_EID = 1,
	RESET_EID,
	SET_DISC_FLAG,
};

union mctp_transport_header {
	struct {
		struct {
			u8 hdr_ver : 4;
			u8 rsvd : 4;
		};
		u8 dst_eid;
		u8 src_eid;
		struct {
			u8 msg_tag : 3;
			u8 tag_owner : 1;
			u8 pkt_seq : 2;
			u8 eom : 1;
			u8 som : 1;
		};
	} __attribute__((packed));
	u32 value;
} __attribute__((packed));

union mctp_msg_header {
	struct {
		struct {
			u8 mt : 7;
			u8 ic : 1;
		};
		u8 msg_type_spec[3];
	} __attribute__((packed));
	u32 value;
} __attribute__((packed));

union mctp_ctrl_msg_header {
	struct {
		struct {
			u8 mt : 7;
			u8 ic : 1;
		};
		struct {
			u8 inst_id : 5;
			u8 rsvd : 1;
			u8 d_bit : 1;
			u8 rq_bit : 1;
		};
		u8 cmd_code;
		u8 cmpl_code;
	} __attribute__((packed));
	u32 value;
} __attribute__((packed));

union mctp_message {
	struct {
		union mctp_msg_header msg_head;
		u8 msg_data[MCTP_MSG_SIZE_MAX - sizeof(union mctp_msg_header)];
	} __attribute__((packed));
	u8 data[MCTP_MSG_SIZE_MAX];
} __attribute__((packed));

union mctp_ctrl_message {
	struct {
		union mctp_ctrl_msg_header ctrl_msg_head;
		u8 msg_data[MCTP_MSG_SIZE_MAX - sizeof(union mctp_ctrl_msg_header)];
	} __attribute__((packed));
	u8 data[MCTP_MSG_SIZE_MAX];
} __attribute__((packed));

union mctp_req_msg_set_eid {
	struct {
		struct {
			u8 oper : 2;
			u8 rsvd : 6;
		};
		u8 eid;
	} __attribute__((packed));
	u8 data[2];
} __attribute__((packed));

#endif // ~ MCTP_H
