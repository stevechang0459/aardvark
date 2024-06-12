#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mctp.h"
#include "smbus.h"

#include "mctp_smbus.h"

static struct mctp_smbus_manager m_mctp_smbus_mgr;

void mctp_smbus_set_handle(int handle)
{
	m_mctp_smbus_mgr.handle = handle;
}

int mctp_smbus_get_handle(void)
{
	return m_mctp_smbus_mgr.handle;
}

int mctp_smbus_transmit_packet(
        u8 dst_slv_addr, union mctp_smbus_packet *pkt, u8 tran_size)
{
	pkt->medi_head.src_slv_addr
	        = m_mctp_smbus_mgr.src_slv_addr << 1 | MCTP_OVER_SMBUS;

	int status = smbus_block_write(
	                     m_mctp_smbus_mgr.handle,
	                     dst_slv_addr,
	                     SMBUS_CMD_CODE_MCTP,
	                     sizeof(pkt->medi_head.src_slv_addr) + tran_size,
	                     pkt->data + 3,
	                     m_mctp_smbus_mgr.pec_enabled);

	if (status) {
		fprintf(stderr, "[%s]: smbus_block_write failed (%d)\n",
		        __FUNCTION__, status);
		return MCTP_ERROR;
	} else
		return MCTP_SUCCESS;
}

int mctp_smbus_init(int handle, u8 src_slv_addr, bool pec_flag)
{
	memset(&m_mctp_smbus_mgr, 0, sizeof(m_mctp_smbus_mgr));

	m_mctp_smbus_mgr.src_slv_addr = src_slv_addr;
	m_mctp_smbus_mgr.pec_enabled = pec_flag;

	if (handle)
		mctp_smbus_set_handle(handle);

	return MCTP_SUCCESS;
}
