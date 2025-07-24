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

void nvme_show_get_features(uint32_t cqedw0)
{
	const char *fid[256] = {
		[NVME_FEAT_FID_TEMP_THRESH] = "Temperature Threshold",
		[NVME_FEAT_FID_POWER_MGMT]  = "Power Management",
	};
	printf("fid                         : %x (%s)\n", nvme_cmd_ctx.fid, fid[nvme_cmd_ctx.fid]);
	const char *sel[8] = {
		[NVME_GET_FEATURES_SEL_CURRENT] = "Current",
		[NVME_GET_FEATURES_SEL_DEFAULT] = "Default",
		[NVME_GET_FEATURES_SEL_SAVED] = "Saved",
		[NVME_GET_FEATURES_SEL_SUPPORTED] = "Supported Capabilities",
	};
	printf("sel                         : %x (%s)\n", nvme_cmd_ctx.sel, sel[nvme_cmd_ctx.sel]);
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

int nvme_get_features(struct aa_args *args, enum nvme_features_id fid, enum nvme_get_features_sel sel)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *req_data = (void *)msg->msg_data;

	req_data->opc                       = nvme_admin_get_features;
	req_data->nsid                      = NVME_NSID_NONE;
	req_data->get_feat.cdw10.fid        = fid;
	req_data->get_feat.cdw10.sel        = sel;
	req_data->get_feat.cdw14.uuid_index = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = req_data->opc;
	nvme_cmd_ctx.fid = fid;
	nvme_cmd_ctx.sel = sel;

	// if (verbose)
	//      print_buf(req_data, sizeof(*req_data), "req_data");

	int ret = nvme_mi_send_admin_command(args, req_data->opc, msg, sizeof(*req_data));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_get_features_power_mgmt(struct aa_args *args, enum nvme_get_features_sel sel)
{
	return nvme_get_features(args, NVME_FEAT_FID_POWER_MGMT, sel);
}

int nvme_get_features_temp_thresh(struct aa_args *args, enum nvme_get_features_sel sel)
{
	return nvme_get_features(args, NVME_FEAT_FID_TEMP_THRESH, sel);
}

int nvme_set_features(struct aa_args *args, enum nvme_features_id fid, uint32_t cdw11, bool sv)
{
	union nvme_mi_msg *msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	struct nvme_mi_adm_req_dw *req_data = (void *)msg->msg_data;

	req_data->opc                       = nvme_admin_set_features;
	req_data->nsid                      = NVME_NSID_NONE;
	req_data->set_feat.cdw10.fid        = fid;
	req_data->set_feat.cdw10.sv         = sv;
	req_data->set_feat.cdw11            = cdw11;
	req_data->set_feat.cdw14.uuid_index = NVME_UUID_NONE;

	nvme_cmd_ctx.opc = req_data->opc;
	nvme_cmd_ctx.fid = fid;

	// if (verbose)
	//      print_buf(req_data, sizeof(*req_data), "req_data");

	int ret = nvme_mi_send_admin_command(args, req_data->opc, msg, sizeof(*req_data));
	if (ret < 0)
		nvme_trace(ERROR, "nvme_mi_send_admin_command failed (%d)\n", ret);

	free(msg);

	return ret;
}

int nvme_set_features_temp_thresh(struct aa_args *args, uint32_t val, bool sv)
{
	return nvme_set_features(args, NVME_FEAT_FID_TEMP_THRESH, val, sv);
}
