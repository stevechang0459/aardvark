#include "nvme.h"
#include "nvme_mi.h"
#include "mctp_transport.h"
#include "mctp_message.h"
#include "crc32.h"
#include "utility.h"
#include "libnvme_types.h"
#include "libnvme_mi_mi.h"
#include "nvme_cmd.h"

#include <stdlib.h>
#include <string.h>

struct nvme_mi_context {
	uint8_t opc;
	bool req_sent;
};

struct nvme_mi_context nvme_mi_ctx = {
	.opc = 0xFF,
	.req_sent = 0
};

int nvme_mi_send_mi_command(uint8_t slv_addr, uint8_t dst_eid, uint8_t csi,
                            union nvme_mi_msg *msg, bool ic)
{
	// msg->nmh.mt = MCTP_MSG_TYPE_NVME_MM;
	// msg->nmh.ic = ic;
	// msg->nmh.csi = csi;
	// msg->nmh.rsvd1 = 0; // Reserved bits should be set to 0
	// msg->nmh.nmimt = NMIMT_MI;
	// msg->nmh.ror = 1; // Request or Response (ROR) set to 1 for request
	return 0;
}

int nvme_mi_send_admin_command(uint8_t slv_addr, uint8_t dst_eid, bool csi, uint8_t opc, union nvme_mi_msg *msg,
                               size_t req_size, bool ic)
{
	nvme_mi_ctx.opc = opc;
	nvme_mi_ctx.req_sent = 1;

	memset(msg, 0, sizeof(msg->nmh));
	uint16_t msg_size = sizeof(msg->nmh) + req_size;

	msg->nmh.mt    = MCTP_MSG_TYPE_NVME_MM;
	msg->nmh.ic    = ic;

	msg->nmh.csi   = csi;
	msg->nmh.rsvd1 = 0;
	msg->nmh.nmimt = NMIMT_ADMIN;
	msg->nmh.ror   = ROR_REQ;
	msg->nmh.meb   = 0;
	msg->nmh.ciap  = 0;
	msg->nmh.rsvd2 = 0;

	print_buf(msg, msg_size, "[%s] msg: %d", __func__, msg_size);

	if (ic)
		msg_size = mctp_message_append_mic(msg, msg_size);

	print_buf(msg, msg_size, "[%s] msg + mic: %d", __func__, msg_size);
	nvme_trace(DEBUG, "crc: %x\n", ~crc32_le_generic(CRC_INIT, msg, msg_size - 4, REVERSED_POLY_CRC32));

	return mctp_transport_send_message(slv_addr, dst_eid, msg, msg_size, rand(), true);
}

int nvme_mi_request_message_handle()
{
	return 0;
}

int nvme_mi_response_message_handle(const union nvme_mi_res_msg *msg, uint16_t size)
{
	if (!nvme_mi_ctx.req_sent)
		return 0;

	nvme_mi_ctx.req_sent = 0;

	const union nvme_mi_res_msg *res_mag = (const union nvme_mi_res_msg *)msg;
	printf("Message Size                : %d\n", size);
	const char *mt[256] = {
		[MCTP_MSG_TYPE_CTRL] = "MCTP Control",
		[MCTP_MSG_TYPE_PLDM] = "PLDM",
		[MCTP_MSG_TYPE_NCSI] = "NC-SI over MCTP",
		[MCTP_MSG_TYPE_ETHERNET] = "Ethernet",
		[MCTP_MSG_TYPE_NVME_MM] = "NVMe MM",
		[MCTP_MSG_TYPE_SPDM] = "SPDM",
		[MCTP_MSG_TYPE_SECURED] = "Secured Messages",
		[MCTP_MSG_TYPE_CXL_FX_API] = "CXL FM API",
		[MCTP_MSG_TYPE_CXL_CCI] = "CXL CCI",
		[MCTP_MSG_TYPE_CXL_VENDOR_PCI] = "Vendor Defined - PCI",
		[MCTP_MSG_TYPE_CXL_VENDOR_IANA] = "Vendor Defined - IANA",
	};
	printf("Message Type                : %d (%s)\n", res_mag->nmh.mt, mt[res_mag->nmh.mt]);
	printf("Integrity Check             : %d\n", res_mag->nmh.ic);
	printf("CSI                         : %d\n", res_mag->nmh.csi);
	const char *nmimt[NMIMT_MAX] = {
		"Control Primitive",
		"NVMe-MI Command",
		"NVMe Admin Command",
		"Reserved",
		"PCIe Command"
	};
	printf("NVMe-MI Message Type        : %d (%s)\n", res_mag->nmh.nmimt, nmimt[res_mag->nmh.nmimt]);
	const char *ror[ROR_MAX] = {
		"Request",
		"Response"
	};
	printf("Request or Response         : %d (%s)\n", res_mag->nmh.ror, ror[res_mag->nmh.ror]);
	printf("MEB                         : %d\n", res_mag->nmh.meb);
	printf("CIAP                        : %d\n", res_mag->nmh.ciap);
	const char *status[256] = {
		[NVME_MI_RESP_SUCCESS] = "Success",
		[NVME_MI_RESP_MPR] = "More Processing Required",
		[NVME_MI_RESP_INTERNAL_ERR] = "Internal Error",
		[NVME_MI_RESP_INVALID_OPCODE] = "Invalid command opcode",
		[NVME_MI_RESP_INVALID_PARAM] = "Invalid command parameter",
		[NVME_MI_RESP_INVALID_CMD_SIZE] = "Invalid command size",
		[NVME_MI_RESP_INVALID_INPUT_SIZE] = "Invalid command input data size",
		[NVME_MI_RESP_ACCESS_DENIED] = "Access Denied",
		[NVME_MI_RESP_VPD_UPDATES_EXCEEDED] = "More VPD updates than allowed",
		[NVME_MI_RESP_PCIE_INACCESSIBLE] = "PCIe functionality currently unavailable",
		[NVME_MI_RESP_MEB_SANITIZED] = "MEB has been cleared due to sanitize",
		[NVME_MI_RESP_ENC_SERV_FAILURE] = "Enclosure services process failed",
		[NVME_MI_RESP_ENC_SERV_XFER_FAILURE] = "Transfer with enclosure services failed",
		[NVME_MI_RESP_ENC_FAILURE] = "Unreoverable enclosure failure",
		[NVME_MI_RESP_ENC_XFER_REFUSED] = "Enclosure services transfer refused",
		[NVME_MI_RESP_ENC_FUNC_UNSUP] = "Unsupported enclosure services function",
		[NVME_MI_RESP_ENC_SERV_UNAVAIL] = "Enclosure services unavailable",
		[NVME_MI_RESP_ENC_DEGRADED] = "Noncritical failure detected by enc. services",
		[NVME_MI_RESP_SANITIZE_IN_PROGRESS] = "Command prohibited during sanitize",
	};
	printf("Status                      : %d (%s)\n", res_mag->nmresp.status, status[res_mag->nmresp.status]);
	printf("NVMe Management Response    : %d\n", res_mag->nmresp.nmresp);
	struct nvme_mi_adm_res_dw *res_data = (void *)res_mag->res_data;
	printf("CQE Dword 0                 : %d\n", res_data->cqedw0);
	printf("CQE Dword 1                 : %d\n", res_data->cqedw1);
	printf("CQE Dword 3                 : %d\n", res_data->cqedw3);

	void *buf = (void *)res_mag->res_data + sizeof(struct nvme_mi_adm_res_dw);
	switch (nvme_mi_ctx.opc) {
	case nvme_admin_get_log_page:
		nvme_show_log_page(buf);
		break;
	case nvme_admin_identify:
		nvme_show_identify(buf);
		break;
	case nvme_admin_get_features:
		nvme_show_get_features(res_data->cqedw0);
		break;
	default:
		nvme_trace(WARN, "unknown mi opc: %d\n", nvme_mi_ctx.opc);
		break;
	}
	nvme_mi_ctx.opc = 0xFF;

	if (res_mag->nmresp.status == 4) {
		printf("Invalid Parameter Error Response:\n");
		printf("  Status                    : %d\n", res_mag->nmresp.invld_para.status);
		printf("  lsbyte                    : %d\n", res_mag->nmresp.invld_para.lsbyte);
		printf("  lsbit                     : %d\n", res_mag->nmresp.invld_para.lsbit);
	}

	return 0;
}

int nvme_mi_message_handle(const union nvme_mi_msg *msg, uint16_t size)
{
	int ret;
	if (msg->nmh.ror == ROR_REQ) {
		ret = nvme_mi_request_message_handle((void *)msg, size);
	} else {
		ret = nvme_mi_response_message_handle((void *)msg, size);
	}

	return ret;
}
