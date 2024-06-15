#include "mctp.h"
#include "mctp_transport.h"
#include "crc32.h"
#include "utility.h"

#include "types.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

static u32 m_inst_id;
static union mctp_ctrl_message *m_mctp_msg = NULL;

u16 mctp_message_append_mic(void *msg, u16 msg_size)
{
	u32 mic = ~crc32_le_generic(CRC_INIT, msg, msg_size, REVERSED_POLY_CRC32);
	memcpy(msg + msg_size, &mic, sizeof(mic));
	return msg_size + sizeof(mic);
}

int mctp_send_control_request_message(
        u8 slv_addr, u8 dst_eid, enum mctp_ctrl_cmd_code cmd_code,
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
	print_buf(msg, msg_size, "[%s]: msg (%d)", __FUNCTION__, msg_size);
	if (ic)
		msg_size = mctp_message_append_mic(msg, msg_size);

	print_buf(msg, msg_size, "[%s]: add mic (%d)", __FUNCTION__, msg_size);
	mctp_trace(DEBUG, "crc: %x\n",
	           ~crc32_le_generic(CRC_INIT, msg, msg_size - 4, REVERSED_POLY_CRC32));

	return mctp_transport_send_message(
	               slv_addr, dst_eid, msg, msg_size,
	               MCTP_MSG_TYPE_NVME_MM, 1);
}

int mctp_message_set_eid(u8 slv_addr, u8 dst_eid, enum set_eid_operation oper,
                         u8 eid, bool ic, bool retry)
{
	int ret;
	union mctp_ctrl_message *msg;
	union mctp_req_msg_set_eid *req_data;

	msg = malloc(sizeof(*msg));

	req_data = (void *)msg->msg_data;
	memset(req_data, 0, sizeof(*req_data));

	mctp_transport_update_addr(slv_addr, eid);
	req_data->oper = oper;
	req_data->eid = eid;

	print_buf(msg->msg_data, sizeof(*req_data), "[%s]: req data (%d)",
	          __FUNCTION__, sizeof(*req_data));

	ret = mctp_send_control_request_message(
	              slv_addr, dst_eid, MCTP_CTRL_MSG_SET_EID, msg,
	              sizeof(*req_data), ic, retry);
	if (ret)
		mctp_trace(ERROR, "mctp_send_control_request_message (%d)\n", ret);

	free(msg);

	return ret;
}

int mctp_message_handle(const union mctp_message *msg, word size)
{
	int status = MCTP_SUCCESS;
	u8 mt = msg->msg_head.mt;

	switch (mt) {
	default:
		break;
	}

	return status;
}

void *mctp_message_alloc(void)
{
	if (!m_mctp_msg)
		m_mctp_msg = malloc(MCTP_MSG_SIZE_MAX);

	return m_mctp_msg;
}

void mctp_message_free(void)
{
	if (m_mctp_msg)
		free(m_mctp_msg);

	m_mctp_msg = NULL;
}

int mctp_message_init(void)
{
	mctp_message_alloc();
	return MCTP_SUCCESS;
}

int mctp_message_deinit(void)
{
	mctp_message_free();
	return MCTP_SUCCESS;
}
