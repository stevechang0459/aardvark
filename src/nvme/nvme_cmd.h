#ifndef NVME_CMD_H
#define NVME_CMD_H

#include "types.h"
#include "libnvme_types.h"

void nvme_show_log_page(const struct nvme_smart_log *smart_log);
void nvme_show_identify(const struct nvme_id_ctrl *id_ctrl);
void nvme_show_get_features(uint32_t cqedw0);

int nvme_get_log_smart(uint8_t slv_addr, uint8_t dst_eid, bool csi, uint32_t namespace_id,
                       bool ic, bool verbose);
int nvme_identify_cns_nsid(uint8_t slv_addr, uint8_t dst_eid, bool csi, uint32_t namespace_id,
                           bool ic, bool verbose);
int nvme_identify_ctrl(uint8_t slv_addr, uint8_t dst_eid, bool csi, bool ic, bool verbose);
int nvme_get_features(uint8_t slv_addr, uint8_t dst_eid, bool csi, enum nvme_features_id fid,
                      enum nvme_get_features_sel sel, bool ic, bool verbose);
int nvme_get_features_power_mgmt(uint8_t slv_addr, uint8_t dst_eid, bool csi, enum nvme_get_features_sel sel,
                                 bool ic, bool verbose);

#endif // NVME_CMD_H