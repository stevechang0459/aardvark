#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "types.h"

#include "mctp.h"
#include "mctp_smbus.h"
#include "mctp_transport.h"

static struct mctp_transport_manager m_mctp_tran_mgr;

int mctp_transport_transmit_packet(
        u8 slave_addr, union mctp_smbus_packet *pkt, const void *payload,
        u8 tran_size, u8 retry, bool eom)
{
	// pkt->tran_head = tran_head.value;
	pkt->tran_head.pkt_seq = m_mctp_tran_mgr.pkt_seq;
	if (!retry) {
		++m_mctp_tran_mgr.pkt_seq;
	}

	if (!m_mctp_tran_mgr.flag.som) {
		m_mctp_tran_mgr.flag.som = 1;
		pkt->tran_head.som = 1;
	} else {
		pkt->tran_head.som = 0;
	}

	if (eom) {
		m_mctp_tran_mgr.flag.eom = 1;
		pkt->tran_head.eom = 1;
	} else {
		pkt->tran_head.eom = 0;
	}

	memcpy(pkt->payload, payload, tran_size);

	m_mctp_tran_mgr.flag.pkt_tmr_en = 1;
	// TBD
	// timer = xxx

	mctp_smbus_transmit_packet(
	        slave_addr, pkt, tran_size + sizeof(pkt->medi_head));

	return MCTP_SUCCESS;
}

int mctp_transport_send_message(
        u8 slave_addr, u8 dst_eid, const void *msg, u16 msg_size, u8 msg_tag,
        u8 tag_owner)
{
	if (msg_size > MCTP_MSG_SIZE_MAX)
		return -MCTP_TRAN_ERR_UNSUP_MSG_SIZE;

	union mctp_smbus_packet *pkt = malloc(sizeof(*pkt));


	if (tag_owner) {
		pkt->tran_head.value = 0;
		pkt->tran_head.hdr_ver = MCTP_HEADER_VERSION;
		pkt->tran_head.dst_eid = dst_eid;
		/**
		 * A source endpoint is allowed to interleave packets from multiple messages
		 * to the same destination endpoint concurrently, provided that each of the
		 * messages has a unique message tag.
		 */
		pkt->tran_head.msg_tag = msg_tag;
	} else {
		// Copy tran_head info from last transaction.
		pkt->tran_head.value = m_mctp_tran_mgr.tran_head.value;
		pkt->tran_head.dst_eid = m_mctp_tran_mgr.tran_head.src_eid;
	}
	pkt->tran_head.src_eid = m_mctp_tran_mgr.eid;
	m_mctp_tran_mgr.flag.value = 0;

	u8 retry = 0;
	while (msg_size) {
		u8 tran_size = msg_size > m_mctp_tran_mgr.nego_size
		               ? m_mctp_tran_mgr.nego_size
		               : msg_size;

		mctp_transport_transmit_packet(
		        slave_addr, pkt, msg, tran_size, retry,
		        msg_size <= m_mctp_tran_mgr.nego_size);

		msg += tran_size;
		msg_size -= tran_size;
	}

	free(pkt);

	return MCTP_SUCCESS;
}

void mctp_tran_init(void)
{
	memset(&m_mctp_tran_mgr, 0, sizeof(m_mctp_tran_mgr));
}
