#include "nvme.h"
#include "nvme_mi.h"
#include "mctp_transport.h"
#include "mctp_message.h"
#include "mctp_core.h"
#include "crc32.h"
#include "utility.h"
#include "libnvme_types.h"
#include "libnvme_mi_mi.h"
#include "nvme_cmd.h"

#include <stdlib.h>
#include <string.h>

struct nvme_mi_context {
	enum nvme_mi_message_type nmimt;
	uint8_t opc;
	bool req_sent;
	enum nvme_mi_config_id cfg_id;
	enum nvme_mi_dtyp dtyp;
};

struct nvme_mi_context nvme_mi_ctx = {
	.nmimt = NVNE_MI_MT_MAX,
	.opc = 0xFF,
	.req_sent = 0
};

int nvme_mi_send_control_primitive()
{
	// return mctp_transport_send_message(slv_addr, dst_eid, msg, msg_size, rand(), true);
	return 0;
}

int nvme_mi_send_command_message(struct aa_args *args, uint8_t opc, enum nvme_mi_message_type nmimt,
                                 union nvme_mi_msg *msg, size_t req_size)
{
	int ret;

	// TBD
	nvme_mi_ctx.nmimt = nmimt;
	nvme_mi_ctx.opc = opc;
	nvme_mi_ctx.req_sent = 1;

	memset(msg, 0, sizeof(msg->nmh));
	uint16_t msg_size = sizeof(msg->nmh) + req_size;

	msg->nmh.mt    = MCTP_MSG_TYPE_NVME_MM;
	msg->nmh.ic    = args->ic;

	msg->nmh.csi   = args->csi;
	msg->nmh.rsvd1 = 0;
	msg->nmh.nmimt = nmimt;
	msg->nmh.ror   = ROR_REQ;
	msg->nmh.meb   = 0;
	msg->nmh.ciap  = 0;
	msg->nmh.rsvd2 = 0;

	if (args->ic)
		msg_size = mctp_message_append_mic(msg, msg_size);

	if (args->verbose) {
		print_buf(msg, msg_size, "[%s] nvme mi command message: %d", __func__, msg_size);
		nvme_trace(DEBUG, "crc: %x\n", ~crc32_le_generic(CRC_INIT, msg, msg_size - 4,
		                                                 REVERSED_POLY_CRC32));
	}

	ret =  mctp_transport_send_message(args->slv_addr, args->dst_eid, msg, msg_size,
	                                   rand(), true, args->verbose);
	if (ret) {
		nvme_trace(ERROR, "mctp_transport_send_message (%d)\n", ret);
		return ret;
	}

#if (!CONFIG_AA_MULTI_THREAD)
	int timeout = args->timeout == -2 ? 1000 : args->timeout;
	ret = smbus_slave_poll(args->handle, timeout, args->pec, mctp_receive_packet_handle,
	                       args->verbose);
	if (ret && ret != 0xFF)
		nvme_trace(ERROR, "smbus_slave_poll (%d)\n", ret);
#endif

	return ret;
}

int nvme_mi_send_mi_command(struct aa_args *args, uint8_t opc, union nvme_mi_msg *msg,
                            size_t req_size)
{
	return nvme_mi_send_command_message(args, opc, NVME_MI_MT_MI, msg, req_size);
}

int nvme_mi_send_admin_command(struct aa_args *args, uint8_t opc, union nvme_mi_msg *msg,
                               size_t req_size)
{
	return nvme_mi_send_command_message(args, opc, NVME_MI_MT_ADMIN, msg, req_size);
}

int nvme_mi_request_message_handle(const union nvme_mi_res_msg *msg, uint16_t size)
{
	return 0;
}

void nvme_mi_show_mi_data_read(void *buf)
{
	switch (nvme_mi_ctx.dtyp) {
	case nvme_mi_dtyp_subsys_info:
		struct nvme_mi_read_nvm_ss_info *nvm_subsys_info = buf;
		printf("NUMP                        : 0x%02x\n", nvm_subsys_info->nump);
		printf("MJR                         : 0x%02x\n", nvm_subsys_info->mjr);
		printf("MNR                         : 0x%02x\n", nvm_subsys_info->mnr);
		break;
	case nvme_mi_dtyp_port_info:
		break;
	case nvme_mi_dtyp_ctrl_list:
		break;
	case nvme_mi_dtyp_ctrl_info:
		break;
	case nvme_mi_dtyp_opt_cmd_support:
		break;
	case nvme_mi_dtyp_meb_support:
		break;
	}
}

void nvme_mi_show_subsystem_health_status(const struct nvme_mi_nvm_ss_health_status *nhsds)
{
	printf("NVM Subsystem Status        : 0x%02x\n", nhsds->nss);
	union nvm_subsys_sts nss = {.value = nhsds->nss};
	printf("  P1LA                      : %d\n", nss.p1la);
	printf("  P0LA                      : %d\n", nss.p0la);
	printf("  RNR                       : %d\n", nss.rnr);
	printf("  DF                        : %d\n", nss.df);
	printf("Smart Warnings              : 0x%02x\n", nhsds->sw);
	union chds_cwarn sw = {.value = ~nhsds->sw};
	printf("  ST                        : %d\n", sw.st);
	printf("  TAUT                      : %d\n", sw.taut);
	printf("  RD                        : %d\n", sw.rd);
	printf("  RO                        : %d\n", sw.ro);
	printf("  VMBF                      : %d\n", sw.vmbf);
	printf("  PMRE                      : %d\n", sw.pmre);
	printf("Composite Temperature       : %d C\n", (char)nhsds->ctemp);
	printf("Percentage Drive Life Used  : %3d %%\n", nhsds->pdlu);
	printf("Composite Controller Status : 0x%02x\n", nhsds->ccs);
	union comp_ctrl_sts ccs = {.value = nhsds->ccs};
	printf("  RDY                       : %d\n", ccs.rdy);
	printf("  CFS                       : %d\n", ccs.cfs);
	printf("  SHST                      : %d\n", ccs.shst);
	printf("  NSSRO                     : %d\n", ccs.nssro);
	printf("  CECO                      : %d\n", ccs.ceco);
	printf("  NAC                       : %d\n", ccs.nac);
	printf("  FA                        : %d\n", ccs.fa);
	printf("  CSTS                      : %d\n", ccs.csts);
	printf("  CTEMP                     : %d\n", ccs.ctemp);
	printf("  PDLU                      : %d\n", ccs.pdlu);
	printf("  SPARE                     : %d\n", ccs.spare);
	printf("  CWARN                     : %d\n", ccs.cwarn);
}

void nvme_mi_show_controller_health_status(const struct nvme_mi_ctrl_health_status *chds)
{
	printf("Controller Identifier       : %d\n", chds->ctlid);
	printf("Controller Status           : 0x%02x\n", chds->csts);
	union chds_csts csts = {.value = chds->csts};
	printf("  RDY                       : %d\n", csts.rdy);
	printf("  CFS                       : %d\n", csts.cfs);
	printf("  SHST                      : %d\n", csts.shst);
	printf("  NSSRO                     : %d\n", csts.nssro);
	printf("  CECO                      : %d\n", csts.ceco);
	printf("  NAC                       : %d\n", csts.nac);
	printf("  FA                        : %d\n", csts.fa);
	printf("Composite Temperature       : %3d K (%3d C)\n", chds->ctemp, chds->ctemp - 273);
	printf("Percentage Used             : %3d %%\n", chds->pdlu);
	printf("Available Spare             : %3d %%\n", chds->spare);
	printf("Critical Warning            : 0x%02x\n", chds->cwarn);
}

void nvme_mi_show_configuration_set(union nvme_mi_resp nmresp)
{

}

char *nvme_mi_show_error_response_configuration_set(union nvme_mi_resp nmresp)
{
	switch (nmresp.status) {
	case NVME_MI_RESP_INVALID_PARAM:
		switch (nmresp.nmresp) {
		case 0x0800:
			return "Configuration Identifier";
		case 0x0900:
			return "SMBus/I2C Frequency";
		case 0x0b00:
			return "Port Identifier";
		case 0x0c00:
			return "MCTP Transmission Unit Size";
		default:
			return "Unknown Error";
		}
		break;
	default:
		return "Unknown Error";
		break;
	}
}

char *nvme_mi_show_error_response_configuration_get(union nvme_mi_resp nmresp)
{
	switch (nmresp.status) {
	case NVME_MI_RESP_INVALID_PARAM:
		switch (nmresp.nmresp) {
		case 0x0800:
			return "Configuration Identifier";
		case 0x0b00:
			return "Port Identifier";
		default:
			return "Unknown Error";
		}
		break;
	default:
		return "Unknown Error";
		break;
	}
}

char *nvme_mi_show_error_response(uint8_t opc, union nvme_mi_resp nmresp)
{
	switch (opc) {
	case nvme_mi_mi_opcode_configuration_set:
		return nvme_mi_show_error_response_configuration_set(nmresp);
	case nvme_mi_mi_opcode_configuration_get:
		return nvme_mi_show_error_response_configuration_get(nmresp);
	default:
		return "Unknown Error";
	}
}

void nvme_mi_show_configuration_get(union nvme_mi_resp nmresp)
{
	switch (nvme_mi_ctx.cfg_id) {
	case NVME_MI_CONFIG_SMBUS_FREQ:
		static const char *i2c_freq[16] = {
			[0] = "SMBus is not supported or is disabled",
			[1] = "100 kHz",
			[2] = "400 kHz",
			[3] = "1 MHz",
			[4 ... 15] = "Reserved",
		};
		printf("SMBus/I2C Frequency:        : 0x%x (%s)\n", nmresp.nmresp, i2c_freq[nmresp.nmresp]);
		break;
	case NVME_MI_CONFIG_MCTP_MTU:
		printf("MCTP Transmission Unit Size : 0x%x (%d bytes)\n", nmresp.nmresp, nmresp.nmresp);
		break;
	case NVME_MI_CONFIG_HEALTH_STATUS_CHANGE:
		printf("NVMe Management Response    : 0x%x\n", nmresp.nmresp);
		break;
	default:
		break;
	}
}

int nvme_mi_response_message_handle(const union nvme_mi_res_msg *msg, uint16_t size)
{
	if (!nvme_mi_ctx.req_sent)
		return 0;

	// if (size < 256) {
	//      print_buf(msg, size, "Response Message");
	// }
	nvme_mi_ctx.req_sent = 0;

	const union nvme_mi_res_msg *res_msg = msg;
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
	printf("Message Type                : %d (%s)\n", res_msg->nmh.mt, mt[res_msg->nmh.mt]);
	printf("Integrity Check             : %d\n", res_msg->nmh.ic);
	printf("CSI                         : %d\n", res_msg->nmh.csi);
	const char *nmimt[NVNE_MI_MT_MAX] = {
		"Control Primitive",
		"NVMe-MI Command",
		"NVMe Admin Command",
		"Reserved",
		"PCIe Command",
		"Asynchronous Event"
	};
	printf("NVMe-MI Message Type        : %d (%s)\n", res_msg->nmh.nmimt, nmimt[res_msg->nmh.nmimt]);
	switch (res_msg->nmh.nmimt) {
	case NVME_MI_MT_MI: {
		const char *opc [256] = {
			[nvme_mi_mi_opcode_mi_data_read] = "Read NVMe-MI Data Structure",
			[nvme_mi_mi_opcode_subsys_health_status_poll] = "NVM Subsystem Health Status Poll",
			[nvme_mi_mi_opcode_controller_health_status_poll] = "Controller Health Status Poll",
			[nvme_mi_mi_opcode_configuration_set] = "Configuration Set",
			[nvme_mi_mi_opcode_configuration_get] = "Configuration Get",
		};
		printf("Opcode                      : %02Xh (%s)\n", nvme_mi_ctx.opc, opc[nvme_mi_ctx.opc]);
		static const char *cfg_id[256] = {
			[0] = "Reserved",
			[NVME_MI_CONFIG_SMBUS_FREQ] = "SMBus/I2C Frequency",
			[NVME_MI_CONFIG_HEALTH_STATUS_CHANGE] = "Health Status Change",
			[NVME_MI_CONFIG_MCTP_MTU] = "MCTP Transmission Unit Size",
			[0x04 ... 0xBF] = "Reserved",
			[0xC0 ... 0xFF] = "Vendor Specific"
		};
		printf("  Configuration Identifier  : %02Xh (%s)\n", nvme_mi_ctx.cfg_id, cfg_id[nvme_mi_ctx.cfg_id]);
	}
	break;
	case NVME_MI_MT_ADMIN: {
		const char *opc [256] = {
			[nvme_admin_get_log_page] = "Get Log Page",
			[nvme_admin_identify] = "Identify",
			[nvme_admin_get_features] = "Get Features",
		};
		printf("Opcode                      : %02Xh (%s)\n", nvme_mi_ctx.opc, opc[nvme_mi_ctx.opc]);
	}
	break;
	default:
		break;
	}
	const char *ror[ROR_MAX] = {
		"Request",
		"Response"
	};
	printf("Request or Response         : %d (%s)\n", res_msg->nmh.ror, ror[res_msg->nmh.ror]);
	printf("MEB                         : %d\n", res_msg->nmh.meb);
	printf("CIAP                        : %d\n", res_msg->nmh.ciap);
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
	printf("Status                      : %d (%s)\n", res_msg->nmresp.status, status[res_msg->nmresp.status]);
	printf("NVMe Management Response    : 0x%x\n", res_msg->nmresp.nmresp);

	switch (nvme_mi_ctx.nmimt) {
	case NVME_MI_MT_MI: {
		if (res_msg->nmresp.status)
			break;

		void *buf = (void *)res_msg->res_data;
		switch (nvme_mi_ctx.opc) {
		case nvme_mi_mi_opcode_mi_data_read:
			nvme_mi_show_mi_data_read(buf);
			break;
		case nvme_mi_mi_opcode_subsys_health_status_poll:
			nvme_mi_show_subsystem_health_status(buf);
			break;
		case nvme_mi_mi_opcode_controller_health_status_poll:
			printf("  Response Entries          : %d\n", res_msg->nmresp.chsp.rent);
			if (res_msg->nmresp.chsp.rent) {
				nvme_mi_show_controller_health_status(buf);
			}
			break;
		case nvme_mi_mi_opcode_configuration_set:
			nvme_mi_show_configuration_set(res_msg->nmresp);
			break;
		case nvme_mi_mi_opcode_configuration_get:
			nvme_mi_show_configuration_get(res_msg->nmresp);
			break;
		}
	}
	break;

	case NVME_MI_MT_ADMIN: {
		struct nvme_mi_adm_res_dw *res_data = (void *)res_msg->res_data;
		printf("CQE Dword 0                 : %d\n", res_data->cqedw0);
		printf("CQE Dword 1                 : %d\n", res_data->cqedw1);
		printf("CQE Dword 3                 : %d\n", res_data->cqedw3);
		void *buf = (void *)res_msg->res_data + sizeof(struct nvme_mi_adm_res_dw);
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
	}
	break;

	default:
		printf("unsupported nmimt: %d\n", nvme_mi_ctx.nmimt);
		break;
	}

	if (res_msg->nmresp.status == NVME_MI_RESP_INVALID_PARAM) {
		printf("Invalid Parameter           : \033[7m%s\033[0m\n", nvme_mi_show_error_response(nvme_mi_ctx.opc, res_msg->nmresp));
		printf("  Status                    : %d\n", res_msg->nmresp.invld_para.status);
		printf("  lsbyte                    : %d\n", res_msg->nmresp.invld_para.lsbyte);
		printf("  lsbit                     : %d\n", res_msg->nmresp.invld_para.lsbit);
	}

	nvme_mi_ctx.opc = 0xFF;

	return 0xFF; // End of Command for T3
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

void nvme_mi_print_msg_header(const union nvme_mi_req_msg *req_msg)
{
	printf("\033[30;43mopc : 0x%08x\033[0m\n", req_msg->opc);
	printf("\033[30;43mnmd0: 0x%08x\033[0m\n", req_msg->nmd0.value);
	printf("\033[30;43mnmd1: 0x%08x\033[0m\n", req_msg->nmd1.value);
}

int nvme_mi_mi_data_read(struct aa_args *args, union nvme_mi_nmd0 nmd0, union nvme_mi_nmd1 nmd1)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc  = nvme_mi_mi_opcode_mi_data_read;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_mi_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_mi_mi_data_read_nvm_subsys_info(struct aa_args *args)
{
	union nvme_mi_nmd0 nmd0 = {
		.rnmds.dtyp = nvme_mi_dtyp_subsys_info,
		.rnmds.portid = 0xFF,
		.rnmds.ctrlid = 0xFFFF,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.dtyp = nmd0.rnmds.dtyp;

	return nvme_mi_mi_data_read(args, nmd0, nmd1);
}

int nvme_mi_mi_subsystem_health_status_poll(struct aa_args *args, bool cs)
{
	union nvme_mi_nmd0 nmd0 = {
		.value = 0,
	};
	union nvme_mi_nmd1 nmd1 = {
		.nshsp.cs = cs,
	};
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc  = nvme_mi_mi_opcode_subsys_health_status_poll;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_mi_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_mi_mi_controller_health_status_poll(struct aa_args *args, bool ccf)
{
	union nvme_mi_nmd0 nmd0 = {
		.chsp.sctlid = 0,
		.chsp.maxrent = 0,
		.chsp.incf = 1,
		.chsp.all = 0,
	};
	union nvme_mi_nmd1 nmd1 = {
		.chsp.csts = 1,
		.chsp.ctemp = 1,
		.chsp.pdlu = 1,
		.chsp.spare = 1,
		.chsp.cwarn = 1,
		.chsp.ccf = ccf,
	};
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc  = nvme_mi_mi_opcode_controller_health_status_poll;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_mi_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_mi_mi_config_get(struct aa_args *args, union nvme_mi_nmd0 nmd0, union nvme_mi_nmd1 nmd1)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc = nvme_mi_mi_opcode_configuration_get;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_mi_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

// Configuration Get - SMBus/I2C Frequency (Configuration Identifier 01h)
int nvme_mi_mi_config_get_sif(struct aa_args *args)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.sif.cfg_id = NVME_MI_CONFIG_SMBUS_FREQ,
		.cfg.sif.port_id = NVME_MI_PORT_ID_SMBUS,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_get(args, nmd0, nmd1);
}

// Configuration Get - Health Status Change (Configuration Identifier 02h)
int nvme_mi_mi_config_get_hsc(struct aa_args *args)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.hsc.cfg_id = NVME_MI_CONFIG_HEALTH_STATUS_CHANGE,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_get(args, nmd0, nmd1);
}

// Configuration Get - MCTP Transmission Unit Size (Configuration Identifier 03h)
int nvme_mi_mi_config_get_mtus(struct aa_args *args)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.mtus.cfg_id = NVME_MI_CONFIG_MCTP_MTU,
		.cfg.mtus.port_id = NVME_MI_PORT_ID_SMBUS,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_get(args, nmd0, nmd1);
}

// Configuration Set
int nvme_mi_mi_config_set(struct aa_args *args, union nvme_mi_nmd0 nmd0, union nvme_mi_nmd1 nmd1)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc = nvme_mi_mi_opcode_configuration_set;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_mi_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

// Configuration Set - SMBus/I2C Frequency (Configuration Identifier 01h)
int nvme_mi_mi_config_set_sif(struct aa_args *args, uint8_t port_id, uint8_t freq_sel)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.sif.cfg_id = NVME_MI_CONFIG_SMBUS_FREQ,
		.cfg.sif.sif = freq_sel,
		.cfg.sif.port_id = port_id,
	};
	union nvme_mi_nmd1 nmd1 = {
		.cfg.value = 0,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_set(args, nmd0, nmd1);
}

// Configuration Set - Health Status Change (Configuration Identifier 02h)
int nvme_mi_mi_config_set_hsc(struct aa_args *args, union nmd1_config_hsc hsc)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.hsc.cfg_id = NVME_MI_CONFIG_HEALTH_STATUS_CHANGE,
	};
	union nvme_mi_nmd1 nmd1 = {
		.cfg.value = hsc.value,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_set(args, nmd0, nmd1);
}

// Configuration Set - MCTP Transmission Unit Size (Configuration Identifier 03h)
int nvme_mi_mi_config_set_mtus(struct aa_args *args, uint8_t port_id, union nmd1_config_mtus mtus)
{
	union nvme_mi_nmd0 nmd0 = {
		.cfg.mtus.cfg_id = NVME_MI_CONFIG_MCTP_MTU,
		.cfg.mtus.port_id = port_id,
	};
	union nvme_mi_nmd1 nmd1 = {
		.cfg.mtus.mtus = mtus.value,
	};

	nvme_mi_ctx.cfg_id = nmd0.cfg.cfg_id;

	return nvme_mi_mi_config_set(args, nmd0, nmd1);
}
