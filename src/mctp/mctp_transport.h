#ifndef MCTP_TRANSPORT_H
#define MCTP_TRANSPORT_H

#include "mctp.h"
#include "mctp_smbus.h"

#include "types.h"
#include <stdbool.h>

#define MCTP_ADDR_MAP_SIZE                              (256)

union mctp_transport_status {
	struct {
		u32 som : 1;
		u32 eom : 1;
		u32 pkt_tmr_en : 1;
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
	u8 tar_eid;
	u8 state;
	u8 retry;
	u8 pkt_seq;
	u8 req_sent;
};

u8 mctp_transport_search_addr(u8 eid);
void mctp_transport_update_addr(u8 addr, u8 eid);
u16 mctp_transport_get_message_size(const union mctp_message *msg);
void mctp_transport_clear_state(dword val);
bool mctp_transport_req_sent(void);
bool mctp_transport_ic_set(const union mctp_message *msg);
bool mctp_transport_som_received(void);
bool mctp_transport_eom_received(void);
int mctp_transport_verify_mic(const union mctp_message *msg);
void mctp_transport_drop_message(u8 clear_flags);
void mctp_transport_show_message(void *msg);
int mctp_transport_assemble_message(union mctp_message *msg,
                                    const union mctp_smbus_packet *pkt);
int mctp_transport_send_message(u8 slave_addr, u8 dst_eid, const void *msg,
                                u16 msg_size, u8 msg_type, u8 tag_owner);
int mctp_transport_check_packet(const union mctp_smbus_packet *pkt);
int mctp_transport_init(u8 owner_eid, u8 tar_eid, u16 nego_size);
int mctp_transport_deinit(void);

#endif
