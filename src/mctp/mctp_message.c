#include "mctp.h"
#include "mctp_message.h"
#include "mctp_transport.h"
#include "crc32.h"
#include "utility.h"

#include "types.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

union mctp_message *g_mctp_req_msg;
union mctp_message *g_mctp_resp_msg;
struct mctp_message_manager m_mctp_msg_mgr;

u16 mctp_message_append_mic(void *msg, u16 msg_size)
{
	u32 mic = ~crc32_le_generic(CRC_INIT, msg, msg_size, REVERSED_POLY_CRC32);
	memcpy(msg + msg_size, &mic, sizeof(mic));
	return msg_size + sizeof(mic);
}

int mctp_send_control_request_message(u8 slv_addr, u8 dst_eid, enum mctp_ctrl_cmd_code cmd_code,
                                      union mctp_ctrl_message *msg, size_t req_size,
                                      bool ic, bool retry)
{
	memset(msg, 0, sizeof(msg->ctrl_msg_head));
	u16 msg_size = sizeof(msg->ctrl_msg_head) + req_size;

	msg->ctrl_msg_head.mt = MCTP_MSG_TYPE_CTRL;
	msg->ctrl_msg_head.ic = ic;

	// Request
	msg->ctrl_msg_head.rq_bit = 1;

	// Datagram
	msg->ctrl_msg_head.d_bit = 0;

	if (!retry)
		++m_mctp_msg_mgr.inst_id;

	msg->ctrl_msg_head.inst_id = m_mctp_msg_mgr.inst_id;
	msg->ctrl_msg_head.cmd_code = cmd_code;

	// m_mctp_msg_mgr.ctrl_msg_head.value = msg->ctrl_msg_head.value;

	print_buf(msg, msg_size, "[%s]: msg (%d)", __func__, msg_size);
	if (ic)
		msg_size = mctp_message_append_mic(msg, msg_size);

	print_buf(msg, msg_size, "[%s]: add mic (%d)", __func__, msg_size);
	mctp_trace(DEBUG, "crc: %x\n", ~crc32_le_generic(CRC_INIT, msg, msg_size - 4, REVERSED_POLY_CRC32));

	return mctp_transport_send_message(slv_addr, dst_eid, msg, msg_size, MCTP_MSG_TYPE_NVME_MM, 1);
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
	          __func__, (u32)sizeof(*req_data));

	ret = mctp_send_control_request_message(slv_addr, dst_eid, MCTP_CTRL_MSG_SET_EID,
	                                        msg, sizeof(*req_data), ic, retry);
	if (ret)
		mctp_trace(ERROR, "mctp_send_control_request_message (%d)\n", ret);

	free(msg);

	return ret;
}

static const char *eid_assign_sts[EID_ASSIGN_STATUS_MAX] = {
	"accepted",
	"rejected",
	"reserved",
	"reserved"
};

static const char *eid_alloc_sts[EID_ALLOC_STATUS_MAX] = {
	"not use an EID pool",
	"requires EID pool",
	"already received an allocation",
	"reserved"
};

u8 mctp_ctrl_resp_set_eid(const union mctp_ctrl_message *msg)
{
	u8 cmpl_code = msg->ctrl_msg_head.cmpl_code;
	const union mctp_resp_data_set_eid *resp_data = (void *)msg->msg_data;

	mctp_trace(INFO, "Set Endpoint ID response message:\n");
	mctp_trace(INFO, "Completion code               : %x\n", cmpl_code);
	mctp_trace(INFO, "Endpoint ID Allocation Status : %s\n", eid_alloc_sts[resp_data->eid_alloc_sts]);
	mctp_trace(INFO, "EID Assignment Status         : %s\n", eid_assign_sts[resp_data->eid_assign_sts]);
	mctp_trace(INFO, "EID Setting                   : %x\n", resp_data->eid_setting);
	mctp_trace(INFO, "EID Pool Size                 : %x\n", resp_data->eid_poll_size);
	mctp_trace(INFO, "\n");

	return cmpl_code;
}

static int mctp_control_request_message_handle(const union mctp_ctrl_message *msg, u16 size)
{
	return MCTP_SUCCESS;
}

static int mctp_control_response_message_handle(const union mctp_ctrl_message *msg, u16 size)
{
	u8 cmpl_code = MCTP_CMPL_SUCCESS;
	u8 cmd_code = msg->ctrl_msg_head.cmd_code;
	// const void *resp_data = msg->msg_data;

	mctp_trace(INFO, "mctp response: %d\n", cmd_code);

	switch (cmd_code) {
	case MCTP_CTRL_MSG_SET_EID:
		cmpl_code = mctp_ctrl_resp_set_eid(msg);
		break;
	}

	return cmpl_code;
}

static int mctp_control_message_handle(const union mctp_ctrl_message *msg, u16 size)
{
	u8 cmpl_code;

	if (msg->ctrl_msg_head.rq_bit) {
		cmpl_code = mctp_control_request_message_handle(msg, size);
		return cmpl_code == MCTP_CMPL_SUCCESS
		       ? MCTP_SUCCESS
		       : MCTP_MSG_ERR_CTRL_REQ_MSG;
	} else {
		cmpl_code = mctp_control_response_message_handle(msg, size);
		return cmpl_code == MCTP_CMPL_SUCCESS
		       ? MCTP_SUCCESS
		       : MCTP_MSG_ERR_CTRL_RESP_MSG;
	}
}

static int mctp_nvme_mm_handle(const union mctp_message *msg, u16 size)
{
	return MCTP_SUCCESS;
}

int mctp_message_handle(const union mctp_message *msg, u16 size)
{
	int ret = MCTP_SUCCESS;
	u8 mt = msg->msg_head.mt;

	switch (mt) {
	case MCTP_MSG_TYPE_CTRL:
		ret = mctp_control_message_handle((void *)msg, size);
		if (ret)
			mctp_trace(ERROR, "mctp_control_message_handle (%d)\n", ret);
		break;

	case MCTP_MSG_TYPE_NVME_MM:
		ret = mctp_nvme_mm_handle((void *)msg, size);
		if (ret)
			mctp_trace(ERROR, "mctp_nvme_mm_handle (%d)\n", ret);
		break;
	}

	return ret;
}

int mctp_message_init(void)
{
	mctp_trace(INIT, "%s\n", __func__);
	memset(&m_mctp_msg_mgr, 0, sizeof(m_mctp_msg_mgr));
	g_mctp_req_msg = malloc(MCTP_MSG_SIZE_MAX);
	g_mctp_resp_msg = malloc(MCTP_MSG_SIZE_MAX);

	return MCTP_SUCCESS;
}

int mctp_message_deinit(void)
{
	mctp_trace(INIT, "%s\n", __func__);
	free(g_mctp_req_msg);
	free(g_mctp_resp_msg);

	return MCTP_SUCCESS;
}
