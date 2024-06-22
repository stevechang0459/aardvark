#ifndef MCTP_MESSAGE_H
#define MCTP_MESSAGE_H

#include "mctp.h"

#include "types.h"
#include <stdbool.h>

// MCTP control message completion codes
enum mctp_ctrl_cmpl_code {
	/**
	 * 0x00 - SUCCESS
	 *
	 * The Request was accepted and completed normally.
	 */
	MCTP_CMPL_SUCCESS = 0,
	/**
	 * 0x01 = ERROR
	 *
	 * This is a generic failure message. (It should not be used when a more
	 * specific result code applies.)
	 */
	MCTP_CMPL_ERROR = 1,
	/**
	 * 0x02 = ERROR_INVALID_DATA
	 *
	 * The packet payload contained invalid data or an illegal parameter value.
	 */
	MCTP_CMPL_ERR_INVLD_DATA,
	/**
	 * 0x03 = ERROR_INVALID_LENGTH
	 *
	 * The message length was invalid. (The Message body was larger or smaller
	 * than expected for the particular request.)
	 */
	MCTP_CMPL_ERR_INVLD_LEN,
	/**
	 * 0x04 = ERROR_NOT_READY
	 *
	 * The Receiver is in a transient state where it is not ready to receive the
	 * corresponding message.
	 */
	MCTP_CMPL_ERR_NOT_RDY,
	/**
	 * 0x05 = ERROR_UNSUPPORTED_CMD
	 *
	 * The command field in the control type of the received message is
	 * unspecified or not supported on this endpoint. This completion code shall
	 * be returned for any unsupported command values received in MCTP control
	 * Request messages.
	 */
	MCTP_CMPL_ERR_UNSUP_CMD,
	/**
	 * 0x80 - 0xFF Command Specific
	 */
	/**
	 * 0x80 = message type number not supported
	 *
	 * Note: Get MCTP version support message
	 */
	MCTP_CMPL_MSG_TYPE_NOT_SUP = 0x80,
};

// EID assignment status
enum eid_assign_status {
	// EID assignment accepted
	EID_ASSIGN_ACCEPT = 0,
	// EID assignment rejected
	EID_ASSIGN_REJECT = 1,
	// Reserved
	EID_ALLOC_STATUS_RSVD1,
	// Reserved
	EID_ALLOC_STATUS_RSVD2,

	EID_ASSIGN_STATUS_MAX
};

// Endpoint ID allocation status
enum eid_alloc_status {
	// Device does not use an EID pool.
	EID_ALLOC_SIMPLE,
	// Endpoint requires EID pool allocation.
	EID_ALLOC_REQ_EID_POOL,
	/**
	 * Endpoint uses an EID pool and has already received an allocation for that
	 * pool.
	 */
	EID_ALLOC_EID_POOL_EXIST,
	// Reserved
	EID_ALLOC_STATUS_RSVD,

	EID_ALLOC_STATUS_MAX
};

union mctp_resp_data_set_eid {
	struct {
		// Byte[0]
		struct {
			// Bit[1:0] Endpoint ID allocation status
			u8 eid_alloc_sts : 2;
			// Bit[3:2] Reserved
			u8 rsvd1 : 2;
			// Bit[5:4] EID assignment status
			u8 eid_assign_sts : 2;
			// Bit[7:6] Reserved
			u8 rsvd2 : 2;
		};
		// Byte[1] EID Setting
		u8 eid_setting;
		// Byte[2] EID Pool Size
		u8 eid_poll_size;
	} __attribute__((packed));
	u8 data[3];
} __attribute__((packed));

struct mctp_message_manager {
	// union mctp_ctrl_msg_header ctrl_msg_head;
	byte inst_id;
};

int mctp_message_set_eid(u8 slv_addr, u8 dst_eid, enum set_eid_operation oper,
                         u8 eid, bool ic, bool retry);
int mctp_message_handle(const union mctp_message *msg, word size);
void *mctp_message_alloc(void);
void mctp_message_free(void);
int mctp_message_init(void);
int mctp_message_deinit(void);

extern union mctp_message *g_mctp_req_msg;
extern union mctp_message *g_mctp_resp_msg;

#endif // ~ MCTP_MESSAGE_H
