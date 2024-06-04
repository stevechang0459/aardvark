#ifndef MCTP_TRANSPORT_H
#define MCTP_TRANSPORT_H

#include "types.h"
#include "mctp.h"

union mctp_transport_status {
	struct {
		u32 som : 1;
		u32 eom : 1;
		u32 pkt_tmr_en : 1;
		u32 req_sent : 1;
	};
	u32 value;
};

struct mctp_transport_manager {
	union mctp_transport_status flag;
	union mctp_transport_header tran_head;
	const u8 *msg;
	/**
	 * For MCTP, the size of a transmission unit is defined as the size of the
	 * packet payload that is carried in an MCTP packet.
	 */
	u16 max_msg_size;
	u16 msg_size;
	u16 plen;
	u8 nego_size;
	u8 tran_size;
	u8 owner_addr;
	u8 owner_eid;
	u8 eid;
	u8 state;
	u8 retry;
	u8 pkt_seq;
};

int mctp_transport_send_message(u8 slave_addr, u8 dst_eid, const void *msg,
                                u16 msg_size, u8 msg_type, u8 tag_owner);

#endif
