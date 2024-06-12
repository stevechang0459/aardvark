#include "mctp_core.h"

#include "mctp.h"
#include "mctp_message.h"
#include "mctp_transport.h"
#include "mctp_smbus.h"

#include "types.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

int mctp_init(
        int handle, u8 owner_eid, u8 tar_eid, u8 src_slv_addr, u16 nego_size,
        bool pec_flag)
{
	int status;

	status = mctp_message_init();
	if (status) {
		fprintf(stderr, "mctp_message_init failed (%d)\n", status);
		goto finish;
	}

	status = mctp_transport_init(owner_eid, tar_eid, nego_size);
	if (status) {
		fprintf(stderr, "mctp_transport_init failed (%d)\n", status);
		goto finish;
	}

	status = mctp_smbus_init(handle, src_slv_addr, pec_flag);
	if (status) {
		fprintf(stderr, "mctp_smbus_init failed (%d)\n", status);
		goto finish;
	}

	return MCTP_SUCCESS;

finish:
	return MCTP_ERROR;
}
