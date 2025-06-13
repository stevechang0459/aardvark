#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mctp.h"
#include "smbus.h"

#include "mctp_smbus.h"

static struct mctp_smbus_context mctp_smbus_ctx;

void mctp_smbus_set_handle(int handle)
{
	mctp_smbus_ctx.handle = handle;
}

int mctp_smbus_get_handle(void)
{
	return mctp_smbus_ctx.handle;
}

int mctp_smbus_check_packet(const union mctp_smbus_header *medi_head)
{
	if ((medi_head->src_slv_addr & 0x1) != MCTP_OVER_SMBUS) {
		smbus_trace(ERROR, "invalid mctp sup\n");
		return -MCTP_SMBUS_ERR_INVLD_SUP;
	}

	if (medi_head->src_slv_addr != (SMBUS_ADDR_IPMI_BMC << 1 | MCTP_OVER_SMBUS)) {
		smbus_trace(WARN, "host is not a bmc controller\n");
	}

	// if (medi_head->src_slv_addr != mctp_smbus_ctx.src_slv_addr)
	// {
	//      fprintf(stderr, "warning: host address is changed\n");
	// }

	return MCTP_SUCCESS;
}

int mctp_smbus_transmit_packet(u8 dst_slv_addr, union mctp_smbus_packet *pkt, u8 tran_size)
{
	if (tran_size > MCTP_TRAN_UNIT_SIZE_MAX)
		return -MCTP_SMBUS_ERR_UNSUP_TRAN_UNIT;

	pkt->medi_head.src_slv_addr = mctp_smbus_ctx.src_slv_addr << 1 | MCTP_OVER_SMBUS;

	int ret = smbus_block_write(mctp_smbus_ctx.handle, dst_slv_addr, SMBUS_CMD_CODE_MCTP,
	                            sizeof(pkt->medi_head.src_slv_addr) + tran_size,
	                            pkt->data + 3, mctp_smbus_ctx.pec_enabled);
	if (ret)
		smbus_trace(ERROR, "smbus_block_write (%d)\n", ret);

	return ret;
}

int mctp_smbus_init(int handle, u8 src_slv_addr, bool pec_flag)
{
	memset(&mctp_smbus_ctx, 0, sizeof(mctp_smbus_ctx));

	mctp_smbus_ctx.src_slv_addr = src_slv_addr;
	mctp_smbus_ctx.pec_enabled  = pec_flag;

	if (handle)
		mctp_smbus_set_handle(handle);

	return MCTP_SUCCESS;
}

int mctp_smbus_deinit(void)
{
	return MCTP_SUCCESS;
}
