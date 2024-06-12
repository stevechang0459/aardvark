#ifndef MCTP_CORE_H
#define MCTP_CORE_H

#include "types.h"
#include <stdbool.h>

int mctp_init(int handle, u8 owner_eid, u8 tar_eid, u8 src_slv_addr,
              u16 nego_size, bool pec_flag);

#endif // ~ MCTP_CORE_H
