#ifndef MCTP_MESSAGE_H
#define MCTP_MESSAGE_H

#include "types.h"
#include "mctp.h"

int mctp_message_set_eid(u8 slave_addr, u8 dst_eid, enum set_eid_operation oper,
                         u8 eid);

#endif
