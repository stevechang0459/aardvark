#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "types.h"

#include "mctp.h"
#include "mctp_smbus.h"
#include "mctp_transport.h"

#include "utility.h"
#include "crc32.h"

static struct mctp_transport_manager mctp_tran_ctx;
static u8 *m_mctp_addr_map;

u8 mctp_transport_search_addr(u8 eid, int verbose)
{
	if (verbose > 1)
		mctp_trace(INFO, "eid:%x, addr:%x\n", eid,  m_mctp_addr_map[eid]);
	return m_mctp_addr_map[eid];
}

void mctp_transport_update_addr(u8 addr, u8 eid)
{
	if (m_mctp_addr_map[eid]) {
		mctp_trace(INFO, "addr %x exist\n", m_mctp_addr_map[eid]);
	}
	m_mctp_addr_map[eid] = addr;
}

int mctp_transport_transmit_packet(u8 slv_addr, union mctp_smbus_packet *pkt,
                                   const void *payload, u8 tran_size, u8 retry,
                                   bool eom, int verbose)
{
	int ret;

	// pkt->tran_head = tran_head.value;
	pkt->tran_head.pkt_seq = mctp_tran_ctx.pkt_seq;
	if (!retry) {
		++mctp_tran_ctx.pkt_seq;
	}

	if (!mctp_tran_ctx.flag.som) {
		mctp_tran_ctx.flag.som = 1;
		pkt->tran_head.som = 1;
	} else {
		pkt->tran_head.som = 0;
	}

	if (eom) {
		mctp_tran_ctx.flag.eom = 1;
		pkt->tran_head.eom = 1;
	} else {
		pkt->tran_head.eom = 0;
	}

	memcpy(pkt->payload, payload, tran_size);

	mctp_tran_ctx.flag.pkt_tmr_en = 1;
	// TBD
	// timer = xxx

	tran_size += sizeof(pkt->tran_head);

	if (verbose)
		print_buf(&pkt->tran_head, tran_size, "[%s] mctp pkt: %d", __func__, tran_size);

	ret = mctp_smbus_transmit_packet(slv_addr, pkt, tran_size, verbose);
	if (ret)
		mctp_trace(ERROR, "mctp_smbus_transmit_packet (%d)\n", ret);

	return ret;
}

int mctp_transport_send_message(u8 slv_addr, u8 dst_eid, const void *msg, u16 msg_size,
                                u8 msg_tag, u8 tag_owner, int verbose)
{
	int ret = -MCTP_ERROR;

	if (msg_size > MCTP_MSG_SIZE_MAX)
		return -MCTP_TRAN_ERR_UNSUP_MSG_SIZE;

	mctp_transport_drop_message(1);

	union mctp_smbus_packet *pkt = malloc(sizeof(*pkt));

	if (tag_owner) {
		pkt->tran_head.value = 0;
		pkt->tran_head.hdr_ver = MCTP_HEADER_VERSION;
		pkt->tran_head.dst_eid = dst_eid;
		/**
		 * A source endpoint is allowed to interleave packets from multiple
		 * messages to the same destination endpoint concurrently, provided that
		 * each of the messages has a unique message tag.
		 */
		pkt->tran_head.msg_tag = msg_tag;
		mctp_tran_ctx.req_sent = 1;
	} else {
		// Copy tran_head info from last transaction.
		pkt->tran_head.value = mctp_tran_ctx.tran_head.value;
		pkt->tran_head.dst_eid = mctp_tran_ctx.tran_head.src_eid;
		// pkt->tran_head.msg_tag = msg_tag;
		mctp_tran_ctx.req_sent = 0;
	}
	pkt->tran_head.src_eid = mctp_tran_ctx.owner_eid;
	pkt->tran_head.tag_owner = tag_owner;

	if (dst_eid)
		slv_addr = mctp_transport_search_addr(dst_eid, verbose);

	u8 retry = 0;
	while (msg_size) {
		u8 tran_size = msg_size > mctp_tran_ctx.nego_size ? mctp_tran_ctx.nego_size : msg_size;
		ret = mctp_transport_transmit_packet(slv_addr, pkt, msg, tran_size, retry,
		                                     msg_size <= mctp_tran_ctx.nego_size,
		                                     verbose);
		if (ret) {
			mctp_trace(ERROR, "mctp_transport_transmit_packet (%d)\n", ret);
			break;
		}

		msg += tran_size;
		msg_size -= tran_size;
	}

	// wait response, keep req_sent set
	mctp_transport_drop_message(1);
	free(pkt);

	return ret;
}

u16 mctp_transport_get_message_size(const union mctp_message *msg)
{
	if (!mctp_tran_ctx.flag.eom)
		return 0;

	if (mctp_tran_ctx.msg_size > 4 && msg->msg_head.ic)
		return mctp_tran_ctx.msg_size - 4;

	return mctp_tran_ctx.msg_size;
}

void mctp_transport_clear_state(u32 val)
{
	mctp_tran_ctx.flag.value &= ~val;
}

bool mctp_transport_req_sent(void)
{
	return mctp_tran_ctx.req_sent;
}

bool mctp_transport_ic_set(const union mctp_message *msg)
{
	return msg->msg_head.ic;
}

bool mctp_transport_som_received(void)
{
	return mctp_tran_ctx.flag.som;
}

bool mctp_transport_eom_received(void)
{
	if (mctp_tran_ctx.flag.eom) {
		mctp_tran_ctx.req_sent = 0;
		return true;
	}
	return false;
}

int mctp_transport_verify_mic(const union mctp_message *msg, int verbose)
{
	if (verbose)
		print_buf(msg, mctp_tran_ctx.msg_size, "verify mic (%d)", mctp_tran_ctx.msg_size);
	const u8 *mic = msg->data + mctp_tran_ctx.msg_size - 4;
	u32 crc1 = ((u32)mic[0]) + ((u32)mic[1] << 8) + ((u32)mic[2] << 16) + ((u32)mic[3] << 24);
	u32 crc2 = ~crc32_le_generic(CRC_INIT, msg, mctp_tran_ctx.msg_size - 4, REVERSED_POLY_CRC32);
	// if (crc1 != crc2)
	if (verbose) {
		mctp_trace(INFO, "mic (%x,%x)\n", crc1, crc2);
	}

	return crc1 == crc2 ? MCTP_SUCCESS : -MCTP_TRAN_ERR_DATA_INTEGRITY;
}

void mctp_transport_drop_message(u8 clear_flags)
{
	mctp_tran_ctx.msg_size = 0;
	if (clear_flags)
		mctp_tran_ctx.flag.value = 0;
}

int mctp_transport_assemble_message(union mctp_message *msg, const union mctp_smbus_packet *pkt, int verbose)
{
	u16 size = mctp_tran_ctx.msg_size;
	u16 plen = mctp_tran_ctx.plen;

	if ((size + plen) > mctp_tran_ctx.max_msg_size) {
		mctp_trace(ERROR, "wrong message size (%d,%d,%d)\n", size, plen,
		           mctp_tran_ctx.max_msg_size);
		return -MCTP_TRAN_ERR_UNSUP_MSG_SIZE;
	}

	if (verbose > 1)
		mctp_trace(INFO, "assemble: %p,%d,%d\n", msg, size, plen);

	memcpy(msg->data + size, pkt->payload, plen);
	mctp_tran_ctx.msg_size = size + plen;
	// print_buf(msg, mctp_tran_ctx.msg_size, "assemble msg (%d)", mctp_tran_ctx.msg_size);

	return MCTP_SUCCESS;
}

int mctp_transport_check_packet(const union mctp_smbus_packet *pkt, int verbose)
{
	const union mctp_transport_header *tran_head = &pkt->tran_head;

	/**
	 * Bad header version: The MCTP header version (Hdr Version) value is not a
	 * value that the endpoint supports.
	 */
	if (tran_head->hdr_ver != MCTP_HEADER_VERSION) {
		mctp_trace(ERROR, "bad header version (%d)\n", tran_head->hdr_ver);
		return -MCTP_TRAN_ERR_BAD_HEADER;
	}

	/**
	 * Unknown destination EID: A packet is received at the physical address of
	 * the device, but the destination EID does not match the EID for the device
	 * or the EID is un-routable.
	 * ---
	 * Null Destination EID: This value indicates that the destination EID value
	 * is to be ignored and that only physical addressing is used to route the
	 * message to the destination on the given bus. This enables communication
	 * with devices that have not been assigned an EID.
	 */
	if ((tran_head->dst_eid != EID_NULL_DST)
	    && (tran_head->dst_eid != mctp_tran_ctx.owner_eid)) {
		mctp_trace(ERROR, "unknown destination EID (%d,%d)\n",
		           tran_head->dst_eid, mctp_tran_ctx.owner_eid);
		return -MCTP_TRAN_ERR_UNKNO_DST_EID;
	}

	u16 plen = pkt->medi_head.byte_cnt - sizeof(pkt->medi_head.src_slv_addr) -
	           sizeof(pkt->tran_head);

	/**
	 * Unsupported transmission unit: The transmission unit size is not
	 * supported by the endpoint that is receiving the packet.
	 */
	if (plen > mctp_tran_ctx.nego_size) {
		mctp_trace(ERROR, "unsupported transmission unit (%d,%d)\n",
		           plen, mctp_tran_ctx.nego_size);

		return -MCTP_TRAN_ERR_UNSUP_TRAN_UNIT;
	}
	if (verbose > 1)
		mctp_trace(INFO, "packet seq: %d,%d\n", tran_head->pkt_seq, mctp_tran_ctx.tran_head.pkt_seq);
	// If this packet is the first packet of a message.
	if (tran_head->som) {
		/**
		 * Receipt of a new "start" packet: Receiving a new "start" packet
		 * (packet with SOM = 1b) for a message to the same message terminus as
		 * a message assembly already in progress will cause the message
		 * assembly in process to be terminated. All data for the message
		 * assembly that was in progress is dropped.
		 */
		if (mctp_tran_ctx.flag.som) {
			mctp_trace(WARN, "receipt of a new start packet\n");
			/**
			 * The newly received start packet is not dropped, but instead it
			 * begins a new message assembly.
			 */
		}
		if (verbose > 1)
			mctp_trace(INFO, "mctp message tag: %d\n", tran_head->msg_tag);

		mctp_transport_drop_message(0);

		mctp_tran_ctx.plen = plen;
		mctp_tran_ctx.flag.som = 1;
		mctp_tran_ctx.flag.eom = 0;
		mctp_tran_ctx.tran_head.value = tran_head->value;
	} else {
		/**
		 * Unexpected "middle" packet: A "middle" packet (SOM flag = 0 and EOM
		 * flag = 0) or "end" packet (SOM flag = 0 and EOM flag = 1) for a
		 * multiple-packet message is received for a given message terminus
		 * without first having received a corresponding "start" packet (where
		 * the "start" packet has SOM flag = 1 and EOM flag = 0) for the
		 * message.
		 */
		if (!mctp_tran_ctx.flag.som) {
			mctp_trace(ERROR, "unexpected middle packet\n");
			return -MCTP_TRAN_ERR_UNEXP_MID_PKT;
		}

		/**
		 * Out-of-sequence packet sequence number: For packets comprising a
		 * given multiple-packet message, the packet sequence number for the
		 * most recently received packet is not a mod 4 increment of the
		 * previously received packet's sequence number. All data for the
		 * message assembly that was in progress is dropped. This is considered
		 * an error condition.
		 */
		if (tran_head->pkt_seq != (mctp_tran_ctx.tran_head.pkt_seq + 1) % 4) {
			mctp_trace(ERROR, "out-of-sequence packet sequence number (%d, %d)\n",
			           tran_head->pkt_seq, mctp_tran_ctx.tran_head.pkt_seq);
			return -MCTP_TRAN_ERR_OOS_PKT_SEQ;
		}
		mctp_tran_ctx.tran_head.pkt_seq = tran_head->pkt_seq;
	}

	/**
	 * Bad, unexpected, or expired message tag: A message with TO bit = 0 was
	 * received, indicating that the destination endpoint was the originator of
	 * the tag value, but the destination endpoint did not originate that value,
	 * or is no longer expecting it. (MCTP bridges do not check message tag or
	 * TO bit values for messages that are not addressed to the bridge’s EID, or
	 * to the bridge’s physical address if null-source or destination-EID
	 * physical addressing is used.)
	 */
	if ((!tran_head->tag_owner && !mctp_tran_ctx.req_sent) ||
	    (tran_head->tag_owner && mctp_tran_ctx.req_sent)) {
		mctp_trace(ERROR, "bad, unexpected, or expired message tag (%d,%d)\n",
		           tran_head->tag_owner, mctp_tran_ctx.req_sent);
		return -MCTP_TRAN_ERR_BAD_MSG_TAG;
	}

	/**
	 * For messages that are split up into multiple packets, the Tag Owner
	 * (TO) and Message Tag bits remain the same for all packets from the
	 * SOM through the EOM.
	 */
	if (!tran_head->som || mctp_tran_ctx.req_sent) {
		if ((tran_head->tag_owner != mctp_tran_ctx.tran_head.tag_owner) ||
		    (tran_head->msg_tag   != mctp_tran_ctx.tran_head.msg_tag)) {
			mctp_trace(ERROR, "inconsistent message tag (%d,%d)(%d,%d)\n",
			           tran_head->tag_owner, tran_head->msg_tag,
			           mctp_tran_ctx.tran_head.tag_owner,
			           mctp_tran_ctx.tran_head.msg_tag);
			return -MCTP_TRAN_ERR_BAD_MSG_TAG2;
		}
	}

	// If this packet is the last packet of a message
	if (tran_head->eom) {
		mctp_tran_ctx.plen = plen;
		// mctp_tran_ctx.req_sent = 0;  // move to mctp_transport_eom_received
		mctp_tran_ctx.flag.som = 0;
		mctp_tran_ctx.flag.eom = 1;
		mctp_tran_ctx.flag.pkt_tmr_en = 0;
		/**
		 * Though the packet sequence number can be any value (0-3) if the SOM
		 * bit is set, it is recommended that it is an increment modulo 4 from
		 * the prior packet with an EOM bit set.
		 */
		mctp_tran_ctx.pkt_seq = tran_head->pkt_seq + 1;
	} else {
		/**
		 * Incorrect transmission unit: An implementation may terminate message
		 * assembly if it receives a "middle" packet (SOM = 0b and EOM = 0b)
		 * where the MCTP packet payload size does not match the MCTP packet
		 * payload size for the start packet (SOM = 1b and EOM bit = 0b). This
		 * is considered an error condition.
		 */
		if (plen != mctp_tran_ctx.plen) {
			mctp_trace(ERROR, "incorrect transmission unit (%d,%d)\n",
			           plen, mctp_tran_ctx.plen);
			return -MCTP_TRAN_ERR_INCORRECT_TRAN_UNIT;
		}
	}

	/**
	 * If the transport layer receives a request message, launch a timer for
	 * detecting message timeout condition.
	 */
	if (tran_head->som && tran_head->tag_owner) {
		mctp_tran_ctx.flag.pkt_tmr_en = 1;
		// mctp_pkt_tmr_1ms = MCTP_SMBUS_MT4_MAX;
	}

	return MCTP_SUCCESS;
}

int mctp_transport_init(u8 owner_eid, u8 tar_eid, u16 nego_size)
{
	srand(time(NULL)); // Seed the random number generator

	mctp_trace(INIT, "%s\n", __func__);
	memset(&mctp_tran_ctx, 0, sizeof(mctp_tran_ctx));

	m_mctp_addr_map = malloc(MCTP_ADDR_MAP_SIZE);
	memset(m_mctp_addr_map, 0, MCTP_ADDR_MAP_SIZE);

	mctp_tran_ctx.owner_eid = owner_eid;
	mctp_tran_ctx.tar_eid = tar_eid;

	mctp_tran_ctx.nego_size = nego_size;
	mctp_tran_ctx.max_msg_size = MCTP_MSG_SIZE_MAX;

	mctp_trace(INIT, "owner = 0x%02x, eid = 0x%02x\n", mctp_tran_ctx.owner_eid,
	           mctp_tran_ctx.tar_eid);
	mctp_trace(INIT, "sizeof(mctp_tran_ctx) = %d\n", (u32)sizeof(mctp_tran_ctx));
	mctp_trace(INIT, "negotiated transfer size = %d\n", mctp_tran_ctx.nego_size);
	mctp_trace(INIT, "maximum message size = %d\n", mctp_tran_ctx.max_msg_size);

	return MCTP_SUCCESS;
}

int mctp_transport_deinit(void)
{
	mctp_trace(INIT, "%s\n", __func__);
	free(m_mctp_addr_map);

	return MCTP_SUCCESS;
}
