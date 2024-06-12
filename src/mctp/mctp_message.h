#ifndef MCTP_MESSAGE_H
#define MCTP_MESSAGE_H

#include "mctp.h"

#include "types.h"
#include <stdbool.h>

int mctp_message_set_eid(u8 slv_addr, u8 dst_eid, enum set_eid_operation oper,
                         u8 eid, bool ic, bool retry);
int mctp_message_init(void);

#endif // ~ MCTP_MESSAGE_H
