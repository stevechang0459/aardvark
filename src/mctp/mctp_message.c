#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "types.h"
#include "mctp.h"
#include "mctp_transport.h"

#include "crc32.h"

static u32 m_inst_id;

u16 mctp_message_append_mic(void *msg, u16 msg_size)
{
	u32 mic = crc32(msg, msg_size);
	memcpy((char *)msg + msg_size, &mic, sizeof(mic));
	return msg_size + sizeof(mic);
}

int mctp_send_control_request_message(
        u8 slave_addr, u8 dst_eid, enum mctp_ctrl_cmd_code cmd_code,
        union mctp_ctrl_message *msg, size_t req_size, bool ic, bool retry)
{
	memset(msg, 0, sizeof(msg->ctrl_msg_head));

	msg->ctrl_msg_head.mt = MCTP_MSG_TYPE_CTRL;
	msg->ctrl_msg_head.ic = ic;

	// Request
	msg->ctrl_msg_head.rq_bit = 1;

	// Datagram
	msg->ctrl_msg_head.d_bit = 0;

	if (!retry)
		++m_inst_id;

	msg->ctrl_msg_head.inst_id = m_inst_id;
	msg->ctrl_msg_head.cmd_code = cmd_code;

	u16 msg_size = sizeof(msg->ctrl_msg_head) + req_size;
	if (ic)
		msg_size = mctp_message_append_mic(msg, msg_size);

	return mctp_transport_send_message(
	               slave_addr, dst_eid, msg, msg_size,
	               MCTP_MSG_TYPE_NVME_MM, 1);
}

int mctp_message_set_eid(u8 slave_addr, u8 dst_eid, enum set_eid_operation oper,
                         u8 eid, bool ic, bool retry)
{
	int status;
	union mctp_ctrl_message *msg;
	union mctp_req_msg_set_eid *req_data;

	msg = malloc(sizeof(*msg));

	req_data = (void *)msg->msg_data;
	memset(req_data, 0, sizeof(*req_data));

	req_data->oper = oper;
	req_data->eid = eid;

	status = mctp_send_control_request_message(
	                 slave_addr, dst_eid, MCTP_CTRL_MSG_SET_EID, msg,
	                 sizeof(*req_data), ic, retry);

	free(msg);

	return status;
}
