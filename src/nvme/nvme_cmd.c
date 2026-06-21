#include "mctp.h"
#include "nvme.h"
#include "nvme_mi.h"
#include "libnvme_types.h"
#include "libnvme_mi_mi.h"

#include "crc32.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct nvme_cmd_context {
	uint8_t opc;
	uint8_t lid;
	uint8_t fid;
	uint8_t sel;
};

struct nvme_cmd_context nvme_cmd_ctx = {
	.opc = 0xFF
};

const char *nvme_trace_header[TRACE_TYPE_MAX] =  {
	"[nvme] error: ",
	"[nvme] warning: ",
	"[nvme] debug: ",
	"[nvme] info: ",
	"[nvme] init: ",
};

void nvme_show_log_page(const struct nvme_smart_log *smart_log)
{
	printf("critical_warning            : %x\n", smart_log->critical_warning);
	printf("temperature                 : %3d K (%3d C)\n", *(word *)smart_log->temperature, *(word *)smart_log->temperature - 273);
	printf("avail_spare                 : %x\n", smart_log->avail_spare);
	printf("spare_thresh                : %x\n", smart_log->spare_thresh);
	printf("percent_used                : %x\n", smart_log->percent_used);
	printf("endu_grp_crit_warn_sumry    : %x\n", smart_log->endu_grp_crit_warn_sumry);
	// printf("rsvd7: %x\n", smart_log->rsvd7[25]);
	printf("data_units_read             : %lld\n", *(qword *)smart_log->data_units_read);
	print_buf(smart_log->data_units_read, sizeof(smart_log->data_units_read), "data_units_read[16]");
	printf("data_units_written          : %lld\n", *(qword *)smart_log->data_units_written);
	print_buf(smart_log->data_units_written, sizeof(smart_log->data_units_written), "data_units_written[16]");
	printf("host_reads                  : %lld\n", *(qword *)smart_log->host_reads);
	print_buf(smart_log->host_reads, sizeof(smart_log->host_reads), "host_reads[16]");
	printf("host_writes                 : %lld\n", *(qword *)smart_log->host_writes);
	print_buf(smart_log->host_writes, sizeof(smart_log->host_writes), "host_writes[16]");
	printf("ctrl_busy_time              : %lld\n", *(qword *)smart_log->ctrl_busy_time);
	print_buf(smart_log->ctrl_busy_time, sizeof(smart_log->ctrl_busy_time), "ctrl_busy_time[16]");
	printf("power_cycles                : %lld\n", *(qword *)smart_log->power_cycles);
	print_buf(smart_log->power_cycles, sizeof(smart_log->power_cycles), "power_cycles[16]");
	printf("power_on_hours              : %lld\n", *(qword *)smart_log->power_on_hours);
	print_buf(smart_log->power_on_hours, sizeof(smart_log->power_on_hours), "power_on_hours[16]");
	printf("unsafe_shutdowns            : %lld\n", *(qword *)smart_log->unsafe_shutdowns);
	print_buf(smart_log->unsafe_shutdowns, sizeof(smart_log->unsafe_shutdowns), "unsafe_shutdowns[16]");
	printf("media_errors                : %lld\n", *(qword *)smart_log->media_errors);
	print_buf(smart_log->media_errors, sizeof(smart_log->media_errors), "media_errors[16]");
	printf("num_err_log_entries         : %lld\n", *(qword *)smart_log->num_err_log_entries);
	print_buf(smart_log->num_err_log_entries, sizeof(smart_log->num_err_log_entries), "num_err_log_entries[16]");
	printf("warning_temp_time           : %d\n", smart_log->warning_temp_time);
	printf("critical_comp_time          : %d\n", smart_log->critical_comp_time);
	printf("Temperature Sensor\n");
	printf("  temp_sensor[0]            : %3d K (%3d C)\n", smart_log->temp_sensor[0], smart_log->temp_sensor[0] - 273);
	printf("  temp_sensor[1]            : %3d K (%3d C)\n", smart_log->temp_sensor[1], smart_log->temp_sensor[1] - 273);
	printf("  temp_sensor[2]            : %3d K (%3d C)\n", smart_log->temp_sensor[2], smart_log->temp_sensor[2] - 273);
	// printf("  temp_sensor[3]            : %d\n", smart_log->temp_sensor[3]);
	// printf("  temp_sensor[4]            : %d\n", smart_log->temp_sensor[4]);
	// printf("  temp_sensor[5]            : %d\n", smart_log->temp_sensor[5]);
	// printf("  temp_sensor[6]            : %d\n", smart_log->temp_sensor[6]);
	// printf("  temp_sensor[7]            : %d\n", smart_log->temp_sensor[7]);
	print_buf(smart_log->temp_sensor, sizeof(smart_log->temp_sensor), "temp_sensor[8]");
	printf("thm_temp1_trans_count       : %x\n", smart_log->thm_temp1_trans_count);
	printf("thm_temp2_trans_count       : %x\n", smart_log->thm_temp2_trans_count);
	printf("thm_temp1_total_time        : %x\n", smart_log->thm_temp1_total_time);
	printf("thm_temp2_total_time        : %x\n", smart_log->thm_temp2_total_time);
	// printf("rsvd232: %x\n", smart_log->rsvd232[280]);
}

void nvme_show_identify(const struct nvme_id_ctrl *id_ctrl)
{
	printf("vid                         : %x\n", id_ctrl->vid);
	printf("ssvid                       : %x\n", id_ctrl->ssvid);
	// printf("sn                          : %x\n", id_ctrl->sn[20]);
	print_buf(id_ctrl->sn, sizeof(id_ctrl->sn), "sn");
	// printf("mn                          : %x\n", id_ctrl->mn[40]);
	print_buf(id_ctrl->mn, sizeof(id_ctrl->mn), "mn");
	// printf("fr                          : %x\n", id_ctrl->fr[8]);
	print_buf(id_ctrl->fr, sizeof(id_ctrl->fr), "fr");
	printf("rab                         : %x\n", id_ctrl->rab);
	// printf("ieee                        : %x\n", id_ctrl->ieee[3]);
	print_buf(id_ctrl->ieee, sizeof(id_ctrl->ieee), "ieee");
	printf("cmic                        : %x\n", id_ctrl->cmic);
	printf("mdts                        : %x\n", id_ctrl->mdts);
	printf("cntlid                      : %x\n", id_ctrl->cntlid);
	printf("ver                         : %x\n", id_ctrl->ver);
	printf("rtd3r                       : %x\n", id_ctrl->rtd3r);
	printf("rtd3e                       : %x\n", id_ctrl->rtd3e);
	printf("oaes                        : %x\n", id_ctrl->oaes);
	printf("ctratt                      : %x\n", id_ctrl->ctratt);
	printf("rrls                        : %x\n", id_ctrl->rrls);
	printf("bpcap                       : %x\n", id_ctrl->bpcap);
	printf("rsvd103                     : %x\n", id_ctrl->rsvd103);
	printf("nssl                        : %x\n", id_ctrl->nssl);
	// printf("rsvd108                     : %x\n", id_ctrl->rsvd108[2]);
	printf("plsi                        : %x\n", id_ctrl->plsi);
	printf("cntrltype                   : %x\n", id_ctrl->cntrltype);
	// printf("fguid                       : %x\n", id_ctrl->fguid[16]);
	print_buf(id_ctrl->fguid, sizeof(id_ctrl->fguid), "fguid");
	printf("crdt1                       : %x\n", id_ctrl->crdt1);
	printf("crdt2                       : %x\n", id_ctrl->crdt2);
	printf("crdt3                       : %x\n", id_ctrl->crdt3);
	printf("crcap                       : %x\n", id_ctrl->crcap);
	// printf("rsvd135                     : %x\n", id_ctrl->rsvd135[118]);
	printf("nvmsr                       : %x\n", id_ctrl->nvmsr);
	printf("vwci                        : %x\n", id_ctrl->vwci);
	printf("mec                         : %x\n", id_ctrl->mec);
	printf("oacs                        : %x\n", id_ctrl->oacs);
	printf("acl                         : %x\n", id_ctrl->acl);
	printf("aerl                        : %x\n", id_ctrl->aerl);
	printf("frmw                        : %x\n", id_ctrl->frmw);
	printf("lpa                         : %x\n", id_ctrl->lpa);
	printf("elpe                        : %x\n", id_ctrl->elpe);
	printf("npss                        : %x\n", id_ctrl->npss);
	printf("avscc                       : %x\n", id_ctrl->avscc);
	printf("apsta                       : %x\n", id_ctrl->apsta);
	printf("wctemp                      : %x\n", id_ctrl->wctemp);
	printf("cctemp                      : %x\n", id_ctrl->cctemp);
	printf("mtfa                        : %x\n", id_ctrl->mtfa);
	printf("hmpre                       : %x\n", id_ctrl->hmpre);
	printf("hmmin                       : %x\n", id_ctrl->hmmin);
	// printf("tnvmcap                     : %x\n", id_ctrl->tnvmcap[16]);
	print_buf(id_ctrl->tnvmcap, sizeof(id_ctrl->tnvmcap), "tnvmcap");
	// printf("unvmcap                     : %x\n", id_ctrl->unvmcap[16]);
	print_buf(id_ctrl->unvmcap, sizeof(id_ctrl->unvmcap), "unvmcap");
	printf("rpmbs                       : %x\n", id_ctrl->rpmbs);
	printf("edstt                       : %x\n", id_ctrl->edstt);
	printf("dsto                        : %x\n", id_ctrl->dsto);
	printf("fwug                        : %x\n", id_ctrl->fwug);
	printf("kas                         : %x\n", id_ctrl->kas);
	printf("hctma                       : %x\n", id_ctrl->hctma);
	printf("mntmt                       : %x\n", id_ctrl->mntmt);
	printf("mxtmt                       : %x\n", id_ctrl->mxtmt);
	printf("sanicap                     : %x\n", id_ctrl->sanicap);
	printf("hmminds                     : %x\n", id_ctrl->hmminds);
	printf("hmmaxd                      : %x\n", id_ctrl->hmmaxd);
	printf("nsetidmax                   : %x\n", id_ctrl->nsetidmax);
	printf("endgidmax                   : %x\n", id_ctrl->endgidmax);
	printf("anatt                       : %x\n", id_ctrl->anatt);
	printf("anacap                      : %x\n", id_ctrl->anacap);
	printf("anagrpmax                   : %x\n", id_ctrl->anagrpmax);
	printf("nanagrpid                   : %x\n", id_ctrl->nanagrpid);
	printf("pels                        : %x\n", id_ctrl->pels);
	printf("domainid                    : %x\n", id_ctrl->domainid);
	printf("kpioc                       : %x\n", id_ctrl->kpioc);
	printf("rsvd359                     : %x\n", id_ctrl->rsvd359);
	printf("mptfawr                     : %x\n", id_ctrl->mptfawr);
	// printf("rsvd362                     : %x\n", id_ctrl->rsvd362[6]);
	// printf("megcap                      : %x\n", id_ctrl->megcap[16]);
	print_buf(id_ctrl->megcap, sizeof(id_ctrl->megcap), "megcap");
	printf("tmpthha                     : %x\n", id_ctrl->tmpthha);
	printf("rsvd385                     : %x\n", id_ctrl->rsvd385);
	printf("cqt                         : %x\n", id_ctrl->cqt);
	// printf("rsvd388                     : %x\n", id_ctrl->rsvd388[124]);
	printf("sqes                        : %x\n", id_ctrl->sqes);
	printf("cqes                        : %x\n", id_ctrl->cqes);
	printf("maxcmd                      : %x\n", id_ctrl->maxcmd);
	printf("nn                          : %x\n", id_ctrl->nn);
	printf("oncs                        : %x\n", id_ctrl->oncs);
	printf("fuses                       : %x\n", id_ctrl->fuses);
	printf("fna                         : %x\n", id_ctrl->fna);
	printf("vwc                         : %x\n", id_ctrl->vwc);
	printf("awun                        : %x\n", id_ctrl->awun);
	printf("awupf                       : %x\n", id_ctrl->awupf);
	printf("icsvscc                     : %x\n", id_ctrl->icsvscc);
	printf("nwpc                        : %x\n", id_ctrl->nwpc);
	printf("acwu                        : %x\n", id_ctrl->acwu);
	printf("ocfs                        : %x\n", id_ctrl->ocfs);
	printf("sgls                        : %x\n", id_ctrl->sgls);
	printf("mnan                        : %x\n", id_ctrl->mnan);
	// printf("maxdna                      : %x\n", id_ctrl->maxdna[16]);
	print_buf(id_ctrl->maxdna, sizeof(id_ctrl->maxdna), "maxdna");
	printf("maxcna                      : %x\n", id_ctrl->maxcna);
	printf("oaqd                        : %x\n", id_ctrl->oaqd);
	printf("rhiri                       : %x\n", id_ctrl->rhiri);
	printf("hirt                        : %x\n", id_ctrl->hirt);
	printf("cmmrtd                      : %x\n", id_ctrl->cmmrtd);
	printf("nmmrtd                      : %x\n", id_ctrl->nmmrtd);
	printf("minmrtg                     : %x\n", id_ctrl->minmrtg);
	printf("maxmrtg                     : %x\n", id_ctrl->maxmrtg);
	printf("trattr                      : %x\n", id_ctrl->trattr);
	printf("rsvd577                     : %x\n", id_ctrl->rsvd577);
	printf("mcudmq                      : %x\n", id_ctrl->mcudmq);
	printf("mnsudmq                     : %x\n", id_ctrl->mnsudmq);
	printf("mcmr                        : %x\n", id_ctrl->mcmr);
	printf("nmcmr                       : %x\n", id_ctrl->nmcmr);
	printf("mcdqpc                      : %x\n", id_ctrl->mcdqpc);
	// printf("rsvd588                     : %x\n", id_ctrl->rsvd588[180]);
	printf("subnqn                      : %x\n", id_ctrl->subnqn[NVME_NQN_LENGTH]);
	// printf("rsvd1024                    : %x\n", id_ctrl->rsvd1024[768]);
	printf("ioccsz                      : %x\n", id_ctrl->ioccsz);
	printf("iorcsz                      : %x\n", id_ctrl->iorcsz);
	printf("icdoff                      : %x\n", id_ctrl->icdoff);
	printf("fcatt                       : %x\n", id_ctrl->fcatt);
	printf("msdbd                       : %x\n", id_ctrl->msdbd);
	printf("ofcs                        : %x\n", id_ctrl->ofcs);
	printf("dctype                      : %x\n", id_ctrl->dctype);
	// printf("rsvd1807                    : %x\n", id_ctrl->rsvd1807[241]);
	// printf(id_ctrl                      : %x"\n", struct id_ctrl->psd[32]);
	// printf("vs                          : %x\n", id_ctrl->vs[1024]);
}

void print_controller_metadata(struct nvme_host_metadata *metadata)
{
	printf("  ndesc                     : %x\n", metadata->ndesc);
	word offset = 0;
	word parsed_cnt = 0;
	while (parsed_cnt < metadata->ndesc) {
		struct nvme_metadata_element_desc *md = (void *)((char *)metadata->descs + offset);
		static const char *type[] = {
			[0x00] = "Reserved",
			[NVME_CTRL_METADATA_OS_CTRL_NAME] = "Operating System Controller Name",
			[NVME_CTRL_METADATA_OS_DRIVER_NAME] = "Operating System Driver Name",
			[NVME_CTRL_METADATA_OS_DRIVER_VER] = "Operating System Driver Version",
			[NVME_CTRL_METADATA_PRE_BOOT_CTRL_NAME] = "Pre-boot Controller Name",
			[NVME_CTRL_METADATA_PRE_BOOT_DRIVER_NAME] = "Pre-boot Driver Name",
			[NVME_CTRL_METADATA_PRE_BOOT_DRIVER_VER] = "Pre-boot Driver Version",
			[NVME_CTRL_METADATA_SYS_PROC_MODEL] = "System Processor Model",
			[NVME_CTRL_METADATA_CHIPSET_DRV_NAME] = "Chipset Driver Name",
			[NVME_CTRL_METADATA_CHIPSET_DRV_VERSION] = "Chipset Driver Version",
			[NVME_CTRL_METADATA_OS_NAME_AND_BUILD] = "Operating System Name and Build",
			[NVME_CTRL_METADATA_SYS_PROD_NAME] = "System Product Name",
			[NVME_CTRL_METADATA_FIRMWARE_VERSION] = "Firmware Version",
			[NVME_CTRL_METADATA_OS_DRIVER_FILENAME] = "Operating System Driver Filename",
			[NVME_CTRL_METADATA_DISPLAY_DRV_NAME] = "Display Driver Name",
			[NVME_CTRL_METADATA_DISPLAY_DRV_VERSION] = "Display Driver Version",
			[NVME_CTRL_METADATA_HOST_DET_FAIL_REC] = "Host-Determined Failure Record",
			[0x18 ... 0x1F] = "Vendor Specific"
		};
		printf("  et                        : %x (%s)\n", md->type, type[md->type]);
		printf("  er                        : %x\n", md->rev);
		printf("  elen                      : %d\n", md->len);
		print_buf(md->val, md->len, "eval:");
		offset += sizeof(*md) + md->len;
		parsed_cnt++;
	}
}
void nvme_show_get_features(uint32_t cqedw0, void *buf)
{
	const char *fid[256] = {
		[NVME_FEAT_FID_TEMP_THRESH] = "Temperature Threshold",
		[NVME_FEAT_FID_POWER_MGMT] = "Power Management",
		[NVME_FEAT_FID_ENH_CTRL_METADATA] = "Enhanced Controller Metadata",
		[NVME_FEAT_FID_CTRL_METADATA] = "Controller Metadata",
		[NVME_FEAT_FID_NS_METADATA]  = "Namespace Metadata",
	};
	printf("fid                         : %x (%s)\n", nvme_cmd_ctx.fid, fid[nvme_cmd_ctx.fid]);
	const char *sel[8] = {
		[NVME_GET_FEATURES_SEL_CURRENT]   = "Current",
		[NVME_GET_FEATURES_SEL_DEFAULT]   = "Default",
		[NVME_GET_FEATURES_SEL_SAVED]     = "Saved",
		[NVME_GET_FEATURES_SEL_SUPPORTED] = "Supported Capabilities",
	};
	printf("sel                         : %x (%s)\n", nvme_cmd_ctx.sel, sel[nvme_cmd_ctx.sel]);

	if (nvme_cmd_ctx.sel == NVME_GET_FEATURES_SEL_SUPPORTED) {
		// static const char *select[] = {
		// 	[0] = "Saveable ",
		// 	[1] = "NS Specific ",
		// 	[2] = "Changeable "
		// };
		printf("cqedw0                      : %x (Saveable%s NS Specific%s Changeable%s)\n", cqedw0,
		       (cqedw0 >> 0) & 0x1 ? "+" : "-",
		       (cqedw0 >> 1) & 0x1 ? "+" : "-",
		       (cqedw0 >> 2) & 0x1 ? "+" : "-");
		return;
	}

	switch (nvme_cmd_ctx.fid) {
	case NVME_FEAT_FID_POWER_MGMT:
		union power_mgmt_cqedw0 {
			struct {
				uint32_t ps   : 5;
				uint32_t wh   : 3;
				uint32_t rsvd : 24;
			};
			uint32_t value;
		};
		union power_mgmt_cqedw0 result = {.value = cqedw0};
		printf("  ps                        : %x\n", result.ps);
		printf("  wh                        : %x\n", result.wh);
		break;
	case NVME_FEAT_FID_ENH_CTRL_METADATA ... NVME_FEAT_FID_CTRL_METADATA:
		print_controller_metadata(buf);
		break;
	case NVME_FEAT_FID_NS_METADATA:
		// print_controller_metadata();
		break;
	}
}

int nvme_get_nsid_log(struct aa_args *args, uint32_t nsid, enum nvme_cmd_get_log_lid lid, bool rae)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *req_data = (void *)msg->msg_data;

	uint32_t numd = (sizeof(struct nvme_smart_log) >> 2) - 1;

	req_data->opc                      = nvme_admin_get_log_page;
	req_data->nsid                     = nsid;
	req_data->get_log_page.cdw10.lid   = lid;
	req_data->get_log_page.cdw10.lsp   = NVME_LOG_LSP_NONE;
	req_data->get_log_page.cdw10.rae   = rae;
	req_data->get_log_page.cdw10.numdl = numd & 0xffff;
	req_data->get_log_page.cdw11.numdu = numd >> 16;
	req_data->get_log_page.cdw11.lsi   = NVME_LOG_LSI_NONE;
	req_data->get_log_page.cdw12.lpol  = 0;
	req_data->get_log_page.cdw13.lpou  = 0;
	req_data->get_log_page.cdw14.uuid  = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = req_data->opc;
	nvme_cmd_ctx.lid = req_data->get_log_page.cdw10.lid;

	// if (verbose)
	//      print_buf(req_data, sizeof(*req_data), "req_data");

	int ret = nvme_mi_send_admin_command(args, req_data->opc, msg, sizeof(*req_data));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_get_log_smart(struct aa_args *args, uint32_t nsid, bool rae)
{
	return nvme_get_nsid_log(args, nsid, NVME_LOG_LID_SMART, rae);
}

int nvme_identify_cns_nsid(struct aa_args *args, uint32_t nsid, uint8_t cns)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *req_data = (void *)msg->msg_data;

	req_data->opc                       = nvme_admin_identify;
	req_data->nsid                      = nsid;
	req_data->identify.cdw10.cns        = NVME_IDENTIFY_CNS_CTRL;
	req_data->identify.cdw10.cntid      = NVME_CNTLID_NONE;
	req_data->identify.cdw11.nvmsetid   = NVME_CNSSPECID_NONE;
	req_data->identify.cdw14.uuid_index = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = req_data->opc;

	// if (verbose)
	//      print_buf(req_data, sizeof(*req_data), "req_data");

	int ret = nvme_mi_send_admin_command(args, req_data->opc, msg, sizeof(*req_data));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_identify_ctrl(struct aa_args *args)
{
	return nvme_identify_cns_nsid(args, NVME_NSID_NONE, NVME_IDENTIFY_CNS_CTRL);
}

int nvme_get_features(struct aa_args *args, enum nvme_features_id fid,
                      enum nvme_get_features_sel sel, uint32_t cdw11)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *adm_req_dw = (void *)msg->msg_data;

	adm_req_dw->opc                       = nvme_admin_get_features;
	adm_req_dw->nsid                      = NVME_NSID_NONE;
	adm_req_dw->get_feat.cdw10.fid        = fid;
	adm_req_dw->get_feat.cdw10.sel        = sel;
	adm_req_dw->get_feat.cdw11.value      = cdw11;
	adm_req_dw->get_feat.cdw14.uuid_index = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = adm_req_dw->opc;
	nvme_cmd_ctx.fid = fid;
	nvme_cmd_ctx.sel = sel;

	if (args->verbose)
		print_buf(adm_req_dw, sizeof(*adm_req_dw), "msg_data");

	int ret = nvme_mi_send_admin_command(args, adm_req_dw->opc, msg, sizeof(*adm_req_dw));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_get_features_power_mgmt(struct aa_args *args, enum nvme_get_features_sel sel)
{
	return nvme_get_features(args, NVME_FEAT_FID_POWER_MGMT, sel, 0);
}

int nvme_get_features_temp_thresh(struct aa_args *args, enum nvme_get_features_sel sel)
{
	return nvme_get_features(args, NVME_FEAT_FID_TEMP_THRESH, sel, 0);
}

int nvme_get_feature_enhanced_controller_metadata(struct aa_args *args, enum nvme_get_features_sel sel, uint32_t gdhm)
{
	return nvme_get_features(args, NVME_FEAT_FID_ENH_CTRL_METADATA, sel, gdhm & 0x1);
}

int nvme_get_feature_controller_metadata(struct aa_args *args, enum nvme_get_features_sel sel, uint32_t gdhm)
{
	return nvme_get_features(args, NVME_FEAT_FID_CTRL_METADATA, sel, gdhm & 0x1);
}

int nvme_get_feature_namespace_metadata(struct aa_args *args, enum nvme_get_features_sel sel, uint32_t gdhm)
{
	return nvme_get_features(args, NVME_FEAT_FID_NS_METADATA, sel, gdhm & 0x1);
}

int nvme_set_features(struct aa_args *args, enum nvme_features_id fid,
                      uint32_t cdw11, bool sv, const void *req_data, dword dlen)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *adm_req_dw = (void *)msg->msg_data;
	dword offset = sizeof(*adm_req_dw);

	adm_req_dw->opc                       = nvme_admin_set_features;
	adm_req_dw->nsid                      = NVME_NSID_NONE;
	adm_req_dw->set_feat.cdw10.fid        = fid;
	adm_req_dw->set_feat.cdw10.sv         = sv;
	adm_req_dw->set_feat.cdw11            = cdw11;
	adm_req_dw->set_feat.cdw14.uuid_index = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = adm_req_dw->opc;
	nvme_cmd_ctx.fid = fid;

	memcpy(msg->msg_data + offset, req_data, dlen);

	if (args->verbose)
		print_buf(msg->msg_data, offset + dlen, "msg_data");

	int ret = nvme_mi_send_admin_command(args, adm_req_dw->opc, msg, offset + dlen);
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_set_features_temp_thresh(struct aa_args *args, uint32_t val, bool sv)
{
	return nvme_set_features(args, NVME_FEAT_FID_TEMP_THRESH, val, sv, NULL, 0);
}

/* Define NVMe Controller Metadata Feature ID */
#define NVME_FEAT_CTRL_METADATA 0x7E

/* The Host Metadata data structure is exactly 4096 bytes */
#define NVME_METADATA_MAX_SIZE 4096

// Element Action (EA)
enum element_action {
	// Add/Update Entry
	ELE_ACT_ADD_UPDATE = 0x0,
	// Delete Entry
	ELE_ACT_DELETE = 0x1,
};

static_assert(sizeof(struct nvme_metadata_element_desc) == 4, "nvme_metadata_element_desc size mismatch");

int test_set_feature_controller_metadata_3(struct aa_args *args)
{
	uint8_t metadata[sizeof(struct nvme_host_metadata)];
	uint32_t offset = 0;
	int ret;

	/**
	 * Step 1: Initialize the entire 4KB buffer to zero.
	 * The NVMe specification requires that unused space after the valid
	 * descriptors must be zero-padded.
	 */
	memset(metadata, 0, sizeof(struct nvme_host_metadata));

	struct nvme_host_metadata *host_meta = (struct nvme_host_metadata *)metadata;
	host_meta->ndesc = 0; /* We are testing 5 diverse descriptors */
	offset += 2;

	struct nvme_metadata_element_desc desc;

	/**
	 * Step 2: Construct descriptor 2 (OS Driver Version - 03h)
	 */
	const char *driver_ver = "";
	uint16_t driver_ver_len = strlen(driver_ver);

	desc.type = NVME_CTRL_METADATA_OS_DRIVER_VER;
	desc.rev  = 0x01;
	desc.len  = driver_ver_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	host_meta->ndesc++;
	printf(TO_STR(NVME_CTRL_METADATA_OS_DRIVER_VER) " (%lld): [%d]\n", sizeof(desc) + desc.len, desc.type);

	/**
	 * Step 3: Construct descriptor 3 (OS Name and Build - 0Ah)
	 */
	const char *os_build = "";
	uint16_t os_build_len = strlen(os_build);

	desc.type = NVME_CTRL_METADATA_OS_NAME_AND_BUILD;
	desc.rev  = 0x01;
	desc.len  = os_build_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	host_meta->ndesc++;
	printf(TO_STR(NVME_CTRL_METADATA_OS_NAME_AND_BUILD) " (%lld): [%d]\n", sizeof(desc) + desc.len, desc.type);

	/**
	 * Step 4: Construct descriptor 3 (OS Name and Build - 0Ah)
	 */
	desc.type = NVME_CTRL_METADATA_SYS_PROD_NAME;
	desc.rev  = 0x01;
	desc.len  = 0;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	host_meta->ndesc++;
	printf(TO_STR(NVME_CTRL_METADATA_SYS_PROD_NAME) " (%lld): [%d]\n", sizeof(desc) + desc.len, desc.type);

	/**
	 * Step 5: Construct descriptor 3 (OS Name and Build - 0Ah)
	 */
	desc.type = NVME_CTRL_METADATA_DISPLAY_DRV_NAME;
	desc.rev  = 0x01;
	desc.len  = 0;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	host_meta->ndesc++;
	printf(TO_STR(NVME_CTRL_METADATA_DISPLAY_DRV_NAME)" (%lld): [%d]\n", sizeof(desc) + desc.len, desc.type);

	/**
	 * Step 6: Issue the Set Features command using the provided API.
	 */
	printf("Sending Controller Metadata (%d bytes used out of 4096)...\n", offset);
	ret = nvme_set_features(args, NVME_FEAT_CTRL_METADATA, (dword)ELE_ACT_DELETE << 13,
	                        false, metadata, sizeof(struct nvme_host_metadata));

	if (ret == 0) {
		printf("Controller Metadata set successfully with %d descriptors!\n", host_meta->ndesc);
	} else {
		printf("Failed to set Controller Metadata, error code: %d\n", ret);
	}

	return ret;
}

int test_set_feature_controller_metadata_2(struct aa_args *args)
{
	uint8_t metadata[sizeof(struct nvme_host_metadata)];
	uint32_t offset = 0;
	int ret;

	/**
	 * Step 1: Initialize the entire 4KB buffer to zero.
	 * The NVMe specification requires that unused space after the valid
	 * descriptors must be zero-padded.
	 */
	memset(metadata, 0, sizeof(struct nvme_host_metadata));

	struct nvme_host_metadata *host_meta = (struct nvme_host_metadata *)metadata;
	host_meta->ndesc = 0; /* We are testing 5 diverse descriptors */
	offset += 2;

	struct nvme_metadata_element_desc desc;

	/**
	 * Step 2: Construct descriptor 2 (OS Driver Version - 03h)
	 */
	const char *driver_ver = "v2.0.2-release";
	uint16_t driver_ver_len = strlen(driver_ver);

	desc.type = NVME_CTRL_METADATA_OS_DRIVER_VER;
	desc.rev  = 0x01;
	desc.len  = driver_ver_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, driver_ver, driver_ver_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Operating System Driver Version (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, driver_ver);

	/**
	 * Step 3: Construct descriptor 3 (OS Name and Build - 0Ah)
	 */
	const char *os_build = "Linux Ubuntu 24.04 (Kernel 7.0.0)";
	uint16_t os_build_len = strlen(os_build);

	desc.type = NVME_CTRL_METADATA_OS_NAME_AND_BUILD;
	desc.rev  = 0x01;
	desc.len  = os_build_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, os_build, os_build_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Operating System Name and Build (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, os_build);

	/**
	 * Step 4: Issue the Set Features command using the provided API.
	 */
	printf("Sending Controller Metadata (%d bytes used out of 4096)...\n", offset);
	ret = nvme_set_features(args, NVME_FEAT_CTRL_METADATA, (dword)ELE_ACT_ADD_UPDATE << 13,
	                        false, metadata, sizeof(struct nvme_host_metadata));

	if (ret == 0) {
		printf("Controller Metadata set successfully with %d descriptors!\n", host_meta->ndesc);
	} else {
		printf("Failed to set Controller Metadata, error code: %d\n", ret);
	}

	return ret;
}

int test_set_feature_controller_metadata(struct aa_args *args)
{
	uint8_t metadata[sizeof(struct nvme_host_metadata)];
	uint32_t offset = 0;
	int ret;

	/**
	 * Step 1: Initialize the entire 4KB buffer to zero.
	 * The NVMe specification requires that unused space after the valid
	 * descriptors must be zero-padded.
	 */
	memset(metadata, 0, sizeof(struct nvme_host_metadata));

	struct nvme_host_metadata *host_meta = (struct nvme_host_metadata *)metadata;
	host_meta->ndesc = 0; /* We are testing 5 diverse descriptors */
	offset += 2;

	struct nvme_metadata_element_desc desc;

	/**
	 * Step 2: Construct descriptor 1 (OS Driver Name - 02h)
	 */
	const char *driver_name = "Apacer NVMe Driver";
	uint16_t driver_name_len = strlen(driver_name);

	desc.type = NVME_CTRL_METADATA_OS_DRIVER_NAME;
	desc.rev  = 0x01;
	desc.len  = driver_name_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, driver_name, driver_name_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Operating System Driver Name (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, driver_name);

	/**
	 * Step 3: Construct descriptor 2 (OS Driver Version - 03h)
	 */
	const char *driver_ver = "v2.0.1-beta";
	uint16_t driver_ver_len = strlen(driver_ver);

	desc.type = NVME_CTRL_METADATA_OS_DRIVER_VER;
	desc.rev  = 0x01;
	desc.len  = driver_ver_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, driver_ver, driver_ver_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Operating System Driver Version (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, driver_ver);

	/**
	 * Step 4: Construct descriptor 3 (OS Name and Build - 0Ah)
	 */
	const char *os_build = "Linux Ubuntu 24.04 (Kernel 6.8.0)";
	uint16_t os_build_len = strlen(os_build);

	desc.type = NVME_CTRL_METADATA_OS_NAME_AND_BUILD;
	desc.rev  = 0x01;
	desc.len  = os_build_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, os_build, os_build_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Operating System Name and Build (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, os_build);
	/**
	 * Step 5: Construct descriptor 4 (Host Firmware Version - 0Ch)
	 */
	const char *fw_ver = "UEFI BIOS v1.23";
	uint16_t fw_ver_len = strlen(fw_ver);

	desc.type = NVME_CTRL_METADATA_FIRMWARE_VERSION;
	desc.rev  = 0x01;
	desc.len  = fw_ver_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, fw_ver, fw_ver_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Firmware Version (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, fw_ver);
	/**
	 * Step 6: Construct descriptor 5 (Host-Determined Failure Record - 10h)
	 * This is useful for injecting a simulated host-side crash context.
	 */
	const char *fail_rec = "NVMe link timeout during heavy IO stress test";
	uint16_t fail_rec_len = strlen(fail_rec);

	desc.type = NVME_CTRL_METADATA_HOST_DET_FAIL_REC;
	desc.rev  = 0x01;
	desc.len  = fail_rec_len;

	memcpy(metadata + offset, &desc, sizeof(desc));
	offset += sizeof(desc);
	memcpy(metadata + offset, fail_rec, fail_rec_len);
	offset += desc.len;
	host_meta->ndesc++;
	printf("Host-Determined Failure Record (%lld): [%d] %s\n", sizeof(desc) + desc.len, desc.type, fail_rec);

	/**
	 * Step 7: Issue the Set Features command using the provided API.
	 */
	printf("Sending Controller Metadata (%d bytes used out of 4096)...\n", offset);
	ret = nvme_set_features(args, NVME_FEAT_CTRL_METADATA, (dword)ELE_ACT_ADD_UPDATE << 13,
	                        false, metadata, sizeof(struct nvme_host_metadata));

	if (ret == 0) {
		printf("Controller Metadata set successfully with %d descriptors!\n", host_meta->ndesc);
	} else {
		printf("Failed to set Controller Metadata, error code: %d\n", ret);
	}

	return ret;
}
