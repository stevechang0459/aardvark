#ifndef MCTP_CORE_H
#define MCTP_CORE_H

#include "types.h"
#include <stdbool.h>

int mctp_receive_packet_handle(const void *buf, u32 len);
int mctp_init(int handle, u8 owner_eid, u8 tar_eid, u8 src_slv_addr,
              u16 nego_size, bool pec_flag);
int mctp_deinit(void);

#endif // ~ MCTP_CORE_H
