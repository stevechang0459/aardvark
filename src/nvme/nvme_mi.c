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
	union nvme_mi_resp nmresp;
	uint8_t opc;
	bool req_sent;
	enum nvme_mi_config_id cfg_id;
	enum nvme_mi_dtyp dtyp;
	uint8_t portid;
	uint8_t ctrlid;
	uint8_t vpd[256];
	uint16_t dofst;
	uint16_t dlen;
};

static const char *_nmimt[NVNE_MI_MT_MAX] = {
	"Control Primitive",
	"NVMe-MI Command",
	"NVMe Admin Command",
	"Reserved",
	"PCIe Command",
	"Asynchronous Event"
};

static const char *_adm_opc[] = {
	[nvme_admin_delete_sq]            = "nvme_admin_delete_sq",
	[nvme_admin_create_sq]            = "nvme_admin_create_sq",
	[nvme_admin_get_log_page]         = "nvme_admin_get_log_page",
	[nvme_admin_delete_cq]            = "nvme_admin_delete_cq",
	[nvme_admin_create_cq]            = "nvme_admin_create_cq",
	[nvme_admin_identify]             = "nvme_admin_identify",
	[nvme_admin_abort_cmd]            = "nvme_admin_abort_cmd",
	[nvme_admin_set_features]         = "nvme_admin_set_features",
	[nvme_admin_get_features]         = "nvme_admin_get_features",
	[nvme_admin_async_event]          = "nvme_admin_async_event",
	[nvme_admin_ns_mgmt]              = "nvme_admin_ns_mgmt",
	[nvme_admin_fw_commit]            = "nvme_admin_fw_commit",
	[nvme_admin_fw_download]          = "nvme_admin_fw_download",
	[nvme_admin_dev_self_test]        = "nvme_admin_dev_self_test",
	[nvme_admin_ns_attach]            = "nvme_admin_ns_attach",
	[nvme_admin_keep_alive]           = "nvme_admin_keep_alive",
	[nvme_admin_directive_send]       = "nvme_admin_directive_send",
	[nvme_admin_directive_recv]       = "nvme_admin_directive_recv",
	[nvme_admin_virtual_mgmt]         = "nvme_admin_virtual_mgmt",
	[nvme_admin_nvme_mi_send]         = "nvme_admin_nvme_mi_send",
	[nvme_admin_nvme_mi_recv]         = "nvme_admin_nvme_mi_recv",
	[nvme_admin_capacity_mgmt]        = "nvme_admin_capacity_mgmt",
	[nvme_admin_discovery_info_mgmt]  = "nvme_admin_discovery_info_mgmt",
	[nvme_admin_fabric_zoning_recv]   = "nvme_admin_fabric_zoning_recv",
	[nvme_admin_lockdown]             = "nvme_admin_lockdown",
	[nvme_admin_fabric_zoning_lookup] = "nvme_admin_fabric_zoning_lookup",
	[nvme_admin_clear_export_nvm_res] = "nvme_admin_clear_export_nvm_res",
	[nvme_admin_fabric_zoning_send]   = "nvme_admin_fabric_zoning_send",
	[nvme_admin_create_export_nvms]   = "nvme_admin_create_export_nvms",
	[nvme_admin_manage_export_nvms]   = "nvme_admin_manage_export_nvms",
	[nvme_admin_manage_export_ns]     = "nvme_admin_manage_export_ns",
	[nvme_admin_manage_export_port]   = "nvme_admin_manage_export_port",
	[nvme_admin_send_disc_log_page]   = "nvme_admin_send_disc_log_page",
	[nvme_admin_track_send]           = "nvme_admin_track_send",
	[nvme_admin_track_receive]        = "nvme_admin_track_receive",
	[nvme_admin_migration_send]       = "nvme_admin_migration_send",
	[nvme_admin_migration_receive]    = "nvme_admin_migration_receive",
	[nvme_admin_ctrl_data_queue]      = "nvme_admin_ctrl_data_queue",
	[nvme_admin_dbbuf]                = "nvme_admin_dbbuf",
	[nvme_admin_fabrics]              = "nvme_admin_fabrics",
	[nvme_admin_format_nvm]           = "nvme_admin_format_nvm",
	[nvme_admin_security_send]        = "nvme_admin_security_send",
	[nvme_admin_security_recv]        = "nvme_admin_security_recv",
	[nvme_admin_sanitize_nvm]         = "nvme_admin_sanitize_nvm",
	[nvme_admin_load_program]         = "nvme_admin_load_program",
	[nvme_admin_get_lba_status]       = "nvme_admin_get_lba_status",
	[nvme_admin_program_act_mgmt]     = "nvme_admin_program_act_mgmt",
	[nvme_admin_mem_range_set_mgmt]   = "nvme_admin_mem_range_set_mgmt",
};

const char *_mi_opc [256] = {
	[nvme_mi_mi_opcode_mi_data_read] = "Read NVMe-MI Data Structure",
	[nvme_mi_mi_opcode_subsys_health_status_poll] = "NVM Subsystem Health Status Poll",
	[nvme_mi_mi_opcode_controller_health_status_poll] = "Controller Health Status Poll",
	[nvme_mi_mi_opcode_configuration_set] = "Configuration Set",
	[nvme_mi_mi_opcode_configuration_get] = "Configuration Get",
	[nvme_mi_mi_opcode_vpd_read] = "VPD Read",
	[nvme_mi_mi_opcode_vpd_write] = "VPD Write",
	[nvme_mi_mi_opcode_reset] = "Reset",
	[nvme_mi_mi_opcode_ses_receive] = "SES Receive",
	[nvme_mi_mi_opcode_ses_send] = "SES Send",
	[nvme_mi_mi_opcode_meb_read] = "Management Endpoint Buffer Read",
	[nvme_mi_mi_opcode_meb_write] = "Management Endpoint Buffer Write",
	[nvme_mi_mi_opcode_shutdown] = "Shutdown",
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
		struct nvme_mi_read_port_info *port_info = buf;
		static const char *portt[256] = {
			[PORT_TYPE_INACTIVE] = "Inactive",
			[PORT_TYPE_PCIE] = "PCIe",
			[PORT_TYPE_SMBUS] = "SMBus",
			[3 ... 0xFF] = "Reserved"
		};

		printf("Port Type                   : %dh (%s)\n", port_info->portt, portt[port_info->portt]);
		printf("Port Capabilities           : %d\n", port_info->rsvd1);
		printf("MTUS                        : %d\n", port_info->mmctptus);
		printf("MEBS                        : %d\n", port_info->meb);
		if (port_info->portt == PORT_TYPE_PCIE) {
			printf("PCIe Maximum Payload Size   : %dh\n", port_info->pcie.mps);
			printf("PCIe Supported Link Speeds  : 0x%02x\n", port_info->pcie.sls);
			printf("PCIe Current Link Speed     : %d\n", port_info->pcie.cls);
			printf("PCIe Maximum Link Width     : %dh\n", port_info->pcie.mlw);
			printf("PCIe Negotiated Link Width  : %d\n", port_info->pcie.nlw);
			printf("PCIe Port Number            : %d\n", port_info->pcie.pn);
		} else if (port_info->portt == PORT_TYPE_SMBUS) {			
			printf("Current VPD SMBus Address   : %02x\n", port_info->smb.vpd_addr);
			printf("Maximum VPD SMBus Frequency : %dh\n", port_info->smb.mvpd_freq);
			printf("Current MEP SMBus Address   : %02x\n", port_info->smb.mme_addr);
			printf("Maximum MEP SMBus Frequency : %dh\n", port_info->smb.mme_freq);
			printf("NVMe Basic Management       : %d\n", port_info->smb.nvmebm);
		}
		break;
	case nvme_mi_dtyp_ctrl_list:
		struct nvme_ctrl_list *ctrl_list = buf;
		printf("Number of Identifiers       : %d\n", ctrl_list->num);
		for (int i = 0; i < ctrl_list->num; i++) {
			printf("  Identifier[%4d]:         : 0x%04x\n", i, ctrl_list->identifier[i]);
		}
		break;
	case nvme_mi_dtyp_ctrl_info:
		struct nvme_mi_read_ctrl_info *ctrl_info = buf;
		printf("Port Identifier             : %d\n", ctrl_info->portid);
		printf("PRII                        : %d\n", ctrl_info->prii);
		printf("PRI                         : %d\n", ctrl_info->pri);
		printf("PCI Vendor ID               : %x\n", ctrl_info->vid);
		printf("PCI Device ID               : %x\n", ctrl_info->did);
		printf("PCI Subsystem Vendor ID     : %x\n", ctrl_info->ssvid);
		printf("PCI Subsystem Device ID     : %x\n", ctrl_info->ssid);
		break;
	case nvme_mi_dtyp_opt_cmd_support:
		struct nvme_mi_read_sc_list *oscl = buf;
		printf("Number of Commands          : %d\n", oscl->numcmd);
		for (int i = 0; i < oscl->numcmd; i++) {
			uint8_t nmimt = oscl->cmds[i].type >> 3;
			uint8_t opc = oscl->cmds[i].opc;
			printf("  [%d] NMIMT                 : %d (%s)\n", i, nmimt, _nmimt[nmimt]);
			if (nmimt == NVME_MI_MT_ADMIN) {
				printf("  [%d] Opcode:               : %d (%s)\n", i, opc, _adm_opc[opc]);
			} else if (nmimt == NVME_MI_MT_MI) {
				printf("  [%d] Opcode:               : %d (%s)\n", i, opc, _mi_opc[opc]);
			}
		}
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

void nvme_mi_show_vpd_read(void *buf, uint16_t size)
{
	struct nvme_mi_vpd_hdr *vpd_hdr = buf;
	printf("Common Header\n");
	printf("  IPMIVER                   : %d\n", vpd_hdr->ipmiver);
	printf("  IUAOFF                    : %d\n", vpd_hdr->iuaoff << 3);
	printf("  CIAOFF                    : %d\n", vpd_hdr->ciaoff << 3);
	printf("  BIAOFF                    : %d\n", vpd_hdr->biaoff << 3);
	printf("  PIAOFF                    : %d\n", vpd_hdr->piaoff << 3);
	printf("  MRIOFF                    : %d\n", vpd_hdr->mrioff << 3);
	printf("  CHCHK                     : %d\n", vpd_hdr->chchk);

	// TBD
	if (vpd_hdr->iuaoff)
	{

	}
	
	if (vpd_hdr->ciaoff)
	{
		
	}
	
	if (vpd_hdr->biaoff)
	{
		
	}
	
	if (vpd_hdr->piaoff)
	{

	}

	if (vpd_hdr->mrioff)
	{
		static const char *_type[256] = {
			[0xB] = "NVMe Record Type ID",
			[0xC] = "NVMe PCIe Port Record Type ID",
			[0xD] = "Topology Record Type ID",
		};
		struct nvme_mi_vpd_mr_common *vpd_mr = buf + (vpd_hdr->mrioff << 3);
		printf("NVMe MultiRecord Area\n");
		printf("  Type                      : %d (%s)\n", vpd_mr->type, _type[vpd_mr->type]);
		printf("  Record Format             : %d\n", vpd_mr->rf);
		printf("  Record Length             : %d\n", vpd_mr->rlen);
		printf("  Record Checksum           : %d\n", vpd_mr->rchksum);
		printf("  Header Checksum           : %d\n", vpd_mr->hchksum);
		printf("  NMRAVN                    : %d\n", vpd_mr->nmra.nmravn);
		printf("  Form Factor               : %d\n", vpd_mr->nmra.ff);
		
		vpd_mr = (void *)vpd_mr + 5 + sizeof(struct nvme_mi_vpd_mra);
		printf("NVMe PCIe Port MultiRecord Area\n");
		printf("  Type                      : %d (%s)\n", vpd_mr->type, _type[vpd_mr->type]);
		printf("  Record Format             : %d\n", vpd_mr->rf);
		printf("  Record Length             : %d\n", vpd_mr->rlen);
		printf("  Record Checksum           : %d\n", vpd_mr->rchksum);
		printf("  Header Checksum           : %d\n", vpd_mr->hchksum);
		printf("  NPPMRAVN                  : %d\n", vpd_mr->ppmra.nppmravn);
		printf("  PCIe Port Number          : %d\n", vpd_mr->ppmra.pn);
		printf("  Port Information          : %d\n", vpd_mr->ppmra.ppi);
		printf("  PCIe Link Speed           : %d\n", vpd_mr->ppmra.ls);
		printf("  PCIe Maximum Link Width   : %x\n", vpd_mr->ppmra.mlw);
		printf("  MCTP Support              : %d\n", vpd_mr->ppmra.mctp);
		printf("  Ref Clk Capability        : %d\n", vpd_mr->ppmra.refccap);
		printf("  Port Identifier           : %d\n", vpd_mr->ppmra.pi);
		
		vpd_mr = (void *)vpd_mr + 5 + sizeof(struct nvme_mi_vpd_ppmra);
		printf("Topology MultiRecord Area\n");
		printf("  Type                      : %d (%s)\n", vpd_mr->type, _type[vpd_mr->type]);
		printf("  Record Format             : %d\n", vpd_mr->rf);
		printf("  Record Length             : %d\n", vpd_mr->rlen);
		printf("  Record Checksum           : %d\n", vpd_mr->rchksum);
		printf("  Header Checksum           : %d\n", vpd_mr->hchksum);
		printf("  Version Number            : %d\n", vpd_mr->tmra.vn);
		printf("  Element Count (N)         : %d\n", vpd_mr->tmra.ec);
	}

	// TBD
	print_buf(buf, size, "VPD Read");
	if (size != nvme_mi_ctx.dlen)
		nvme_trace(WARN, "dlen mismatch: size(%d) != dlen(%d)\n", size, nvme_mi_ctx.dlen);
	memcpy(nvme_mi_ctx.vpd + nvme_mi_ctx.dofst, buf, size);
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
	printf("NVMe-MI Message Type        : %d (%s)\n", res_msg->nmh.nmimt, _nmimt[res_msg->nmh.nmimt]);
	switch (res_msg->nmh.nmimt) {
	case NVME_MI_MT_MI: {
		printf("Opcode                      : %02Xh (%s)\n", nvme_mi_ctx.opc, _mi_opc[nvme_mi_ctx.opc]);
		if (nvme_mi_ctx.opc == nvme_mi_mi_opcode_configuration_set || nvme_mi_ctx.opc == nvme_mi_mi_opcode_configuration_get) {
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
		if (nvme_mi_ctx.opc == nvme_mi_mi_opcode_mi_data_read) {
			static const char *dtyp[256] = {
				[nvme_mi_dtyp_subsys_info] = "NVM Subsystem Information",
				[nvme_mi_dtyp_port_info] = "Port Information",
				[nvme_mi_dtyp_ctrl_list] = "Controller List",
				[nvme_mi_dtyp_ctrl_info] = "Controller Information",
				[nvme_mi_dtyp_opt_cmd_support] = "Optionally Supported Command List",
				[nvme_mi_dtyp_meb_support] = "Management Endpoint Buffer Command Support List",
				[0x06 ... 0xFF] = "Reserved"
			};
			printf("  Data Structure Type       : %02Xh (%s)\n", nvme_mi_ctx.dtyp, dtyp[nvme_mi_ctx.dtyp]);
			if (nvme_mi_ctx.dtyp == nvme_mi_dtyp_port_info) {
				printf("  Port Identifier           : %d\n", nvme_mi_ctx.portid);
			}
			if (nvme_mi_ctx.dtyp == nvme_mi_dtyp_ctrl_list) {
				printf("  Controller Identifier     : %d\n", nvme_mi_ctx.ctrlid);
			}
		}
	}
	break;
	case NVME_MI_MT_ADMIN: {
		printf("Opcode                      : %02Xh (%s)\n", nvme_mi_ctx.opc, _adm_opc[nvme_mi_ctx.opc]);
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
		if (res_msg->nmresp.status) {
			nvme_mi_ctx.nmresp.status = res_msg->nmresp.status;
			break;
		}
		nvme_mi_ctx.nmresp.status = NVME_MI_RESP_SUCCESS;

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
		case nvme_mi_mi_opcode_vpd_read:
			nvme_mi_show_vpd_read(buf, size - sizeof(res_msg->nmh) - sizeof(res_msg->nmresp));
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

int nvme_mi_mi_data_read_port_info(struct aa_args *args, uint8_t portid)
{
	union nvme_mi_nmd0 nmd0 = {
		.rnmds.dtyp = nvme_mi_dtyp_port_info,
		.rnmds.portid = portid,
		.rnmds.ctrlid = 0xFFFF,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.dtyp = nmd0.rnmds.dtyp;
	nvme_mi_ctx.portid = portid;

	return nvme_mi_mi_data_read(args, nmd0, nmd1);
}

int nvme_mi_mi_data_read_ctrl_list(struct aa_args *args, uint8_t ctrlid)
{
	union nvme_mi_nmd0 nmd0 = {
		.rnmds.dtyp = nvme_mi_dtyp_ctrl_list,
		.rnmds.ctrlid = ctrlid,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.dtyp = nmd0.rnmds.dtyp;
	nvme_mi_ctx.ctrlid = ctrlid;

	return nvme_mi_mi_data_read(args, nmd0, nmd1);
}

int nvme_mi_mi_data_read_ctrl_info(struct aa_args *args, uint8_t ctrlid)
{
	union nvme_mi_nmd0 nmd0 = {
		.rnmds.dtyp = nvme_mi_dtyp_ctrl_info,
		.rnmds.ctrlid = ctrlid,
	};
	union nvme_mi_nmd1 nmd1 = {
		.value = 0,
	};

	nvme_mi_ctx.dtyp = nmd0.rnmds.dtyp;
	nvme_mi_ctx.ctrlid = ctrlid;

	return nvme_mi_mi_data_read(args, nmd0, nmd1);
}

int nvme_mi_mi_data_read_opt_cmd_support(struct aa_args *args, uint8_t ctrlid, uint8_t iocsi)
{
	union nvme_mi_nmd0 nmd0 = {
		.rnmds.dtyp = nvme_mi_dtyp_opt_cmd_support,
		.rnmds.ctrlid = ctrlid,
	};
	union nvme_mi_nmd1 nmd1 = {
		.rnmds.iocsi = iocsi,
	};

	nvme_mi_ctx.dtyp = nmd0.rnmds.dtyp;
	nvme_mi_ctx.ctrlid = ctrlid;

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

int nvme_mi_mi_vpd_read(struct aa_args *args, uint16_t dofst, uint16_t dlen, void *buf)
{
	union nvme_mi_nmd0 nmd0 = {
		.vpdr.dofst = dofst,
	};
	union nvme_mi_nmd1 nmd1 = {
		.vpdr.dlen = dlen,
	};
	memset(nvme_mi_ctx.vpd, 0, sizeof(nvme_mi_ctx.vpd));
	nvme_mi_ctx.dofst = dofst;
	nvme_mi_ctx.dlen = dlen;
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	union nvme_mi_req_msg *req_msg = (void *)msg;

	req_msg->opc  = nvme_mi_mi_opcode_vpd_read;
	req_msg->nmd0 = nmd0;
	req_msg->nmd1 = nmd1;
	nvme_mi_print_msg_header(req_msg);

	int ret = nvme_mi_send_mi_command(args, req_msg->opc, msg, sizeof(union nvme_mi_req_dw) - sizeof(union nvme_mi_msg_header));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_mi_vpd_read failed (%d)\n", ret);

	if (!nvme_mi_ctx.nmresp.status && buf)
		memcpy(buf, nvme_mi_ctx.vpd + dofst, dlen);

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
