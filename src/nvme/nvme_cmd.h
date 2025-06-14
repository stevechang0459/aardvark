#ifndef NVME_CMD_H
#define NVME_CMD_H

#include "types.h"
#include "libnvme_types.h"

void nvme_show_log_page(const struct nvme_smart_log *smart_log);
void nvme_show_identify(const struct nvme_id_ctrl *id_ctrl);
void nvme_show_get_features(uint32_t cqedw0);

int nvme_get_log_smart(struct aa_args *args, uint32_t nsid, bool rae);
int nvme_identify_ctrl(struct aa_args *args);
int nvme_get_features_power_mgmt(struct aa_args *args, enum nvme_get_features_sel sel);

#endif // NVME_CMD_H