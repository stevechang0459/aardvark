#include "mctp_core.h"

#include "mctp.h"
#include "mctp_message.h"
#include "mctp_transport.h"
#include "mctp_smbus.h"

#include "types.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

const char *mctp_trace_header[TRACE_TYPE_MAX] =  {
	"mctp: error: ",
	"mctp: warning: ",
	"mctp: debug: ",
	"mctp: info: ",
	"mctp: init: ",
};

static union mctp_message *m_mctp_msg;

int mctp_receive_packet_handle(const void *buf, u32 len)
{
	int ret;
	union mctp_message *msg = m_mctp_msg;
	const union mctp_smbus_packet *pkt = buf;

	ret = mctp_smbus_check_packet(&pkt->medi_head);
	if (unlikely(ret != MCTP_SUCCESS)) {
		mctp_trace(ERROR, "mctp_smbus_check_packet (%d)\n", ret);
		return ret;
	}

	ret = mctp_transport_check_packet(pkt);
	if (unlikely(ret != MCTP_SUCCESS)) {
		mctp_trace(ERROR, "mctp_transport_check_packet (%d)\n", ret);
		return ret;
	}

	ret = mctp_transport_assemble_message(msg, pkt);
	if (ret) {
		mctp_transport_drop_message(1);
		mctp_trace(ERROR, "mctp_transport_assemble_message (%d)\n", ret);
		return ret;
	}

	if (mctp_transport_eom_received()) {
		if (mctp_transport_ic_set(msg)) {
			/**
			 * Bad message integrity check: For single- or multiple-packet
			 * messages that use a message integrity check, a mismatch with the
			 * message integrity check value can cause the message assembly to
			 * be terminated and the entire message to be dropped, unless it is
			 * overridden by the specification for a particular message type.
			 */
			ret = mctp_transport_verify_mic(msg);
			if (unlikely(ret != MCTP_SUCCESS)) {
				mctp_trace(ERROR, "bad mic\n");
				return ret;
			} else {
				mctp_trace(INFO, "good mic\n");
			}
		}

		u16 msg_size = mctp_transport_get_message_size(msg);
		ret = mctp_message_handle(msg, msg_size);
		if (ret) {
			mctp_trace(ERROR, "mctp_message_handle (%d)\n", ret);
			return ret;
		}
	}

	return MCTP_SUCCESS;
}

int mctp_init(int handle, u8 owner_eid, u8 tar_eid, u8 src_slv_addr,
              u16 nego_size, bool pec_flag)
{
	int ret;

	ret = mctp_message_init();
	if (ret) {
		mctp_trace(ERROR, "mctp_message_init (%d)\n", ret);
		return ret;
	}

	ret = mctp_transport_init(owner_eid, tar_eid, nego_size);
	if (ret) {
		mctp_trace(ERROR, "mctp_transport_init (%d)\n", ret);
		return ret;
	}

	ret = mctp_smbus_init(handle, src_slv_addr, pec_flag);
	if (ret) {
		mctp_trace(ERROR, "mctp_smbus_init (%d)\n", ret);
		return ret;
	}

	m_mctp_msg = mctp_message_alloc();

	return MCTP_SUCCESS;
}

int mctp_deinit(void)
{
	mctp_message_deinit();
	mctp_transport_deinit();
	mctp_smbus_deinit();

	return MCTP_SUCCESS;
}
