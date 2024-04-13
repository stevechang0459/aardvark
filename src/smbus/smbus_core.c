#include <stdio.h>
#include <stdlib.h>

#include "smbus_core.h"
#include "aardvark.h"
#include "types.h"
#include "crc.h"

static u8 block[BLOCK_SIZE_MAX];

int smbus_write_block(Aardvark handle, u8 tar_addr, u8 cmd_code, const u8 *block, u8 len, u8 pec)
{
	return 0;
}

int smbus_write_file(Aardvark handle, u8 tar_addr, u8 cmd_code,
                     const char *file_name, u8 pec)
{
	FILE *file;

	// Open the file
	file = fopen(file_name, "rb");
	if (!file) {
		printf("error: unable to open file '%s'\n", file_name);
		return -1;
	}

	while (!feof(file)) {
		int write_count, tx_count, byte_count;
		int i;

		// Read from the file
		byte_count = fread((void *)&block[3], 1, BLOCK_SIZE_MAX, file);
		fprintf(stderr, "byte_count=%d\n", byte_count);
		if (!byte_count)
			break;

		block[0] = tar_addr;
		block[1] = cmd_code;
		block[2] = byte_count;
		// data_out[3:byte_count + 2]
		write_count = byte_count + 2; // (cmd_code & byte_count)

		if (pec) {
			write_count++;
			block[write_count] = crc8(0, block, write_count);
		}

		// Write the data to the bus
		tx_count = aa_i2c_write(handle, tar_addr >> 1, AA_I2C_NO_FLAGS,
		                        (u16)write_count, &block[1]);
		if (tx_count < 0) {
			printf("error: %s\n", aa_status_string(tx_count));
			goto cleanup;
		} else if (tx_count == 0) {
			printf("error: no bytes written\n");
			printf("  are you sure you have the right slave address?\n");
			goto cleanup;
		} else if (tx_count != write_count) {
			printf("error: only a partial number of bytes written\n");
			printf("  (%d) instead of full (%d)\n", tx_count, write_count);
			goto cleanup;
		} else {
			fprintf(stderr, "wr:%d,tx:%d\n", tx_count, write_count);
		}

		// Dump the data to the screen
		printf("Data written to device:");
		for (i = 0; i < tx_count; ++i) {
			if ((i & 0x0f) == 0) {
				printf("\n%04x:  ", i);
			}
			printf("%02x ", block[i + 1] & 0xff);
			if (((i + 1) & 0x07) == 0) {
				printf(" ");
			}
		}
		printf("\n\n");

		// Sleep a tad to make sure slave has time to process this request
		aa_sleep_ms(10);
	}

cleanup:
	fclose(file);

	return 0;
}
