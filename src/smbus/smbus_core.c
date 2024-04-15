#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "smbus_core.h"
#include "aardvark.h"
#include "types.h"
#include "crc.h"

static u8 packet[PACKET_SIZE_MAX];

void dump_packet(const u8 *packet, int packet_len, const char *fmt, ...)
{
	if (fmt) {
		va_list argp;
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		fputc('\n', stderr);
	}

	for (int i = 0; i < packet_len; ++i) {
		if ((i & 0x0f) == 0) {
			printf("\n%04x:  ", i);
		}
		printf("%02x ", packet[i + 1] & 0xff);
		if (((i + 1) & 0x07) == 0) {
			printf(" ");
		}
	}
	printf("\n\n");
}

int smbus_write_block(Aardvark handle, u8 tar_addr, u8 cmd_code,
                      const u8 *block, u8 byte_count, u8 pec)
{
	int write_count, tx_count;
	packet[0] = tar_addr;
	packet[1] = cmd_code;
	packet[2] = byte_count;
	memcpy((void *)&packet[3], block, byte_count);
	write_count = byte_count + 2;

	if (pec) {
		write_count++;
		packet[write_count] = crc8(0, packet, write_count);
	}

	// Write the data to the bus
	tx_count = aa_i2c_write(handle, tar_addr >> 1, AA_I2C_NO_FLAGS,
	                        (u16)write_count, &packet[1]);
	if (tx_count < 0) {
		fprintf(stderr, "error: %s\n", aa_status_string(tx_count));
		return 0;
	} else if (tx_count == 0) {
		fprintf(stderr, "error: no bytes written\n");
		fprintf(stderr, "  are you sure you have the right slave address?\n");
		return 0;
	} else if (tx_count != write_count) {
		fprintf(stderr, "error: only a partial number of bytes written\n");
		fprintf(stderr, "  (%d) instead of full (%d)\n", tx_count, write_count);
		return 0;
	} else
		fprintf(stderr, "wr:%d,tx:%d\n", write_count, tx_count);

	// Dump the data to the screen
	dump_packet(packet, tx_count, "Data written to device:");

	return 0;
}

int smbus_write_file(Aardvark handle, u8 tar_addr, u8 cmd_code,
                     const char *file_name, u8 pec)
{
	// Open the file
	FILE *file = fopen(file_name, "rb");
	if (!file) {
		perror("fopen");
		return -1;
	}

	while (!feof(file)) {
		int write_count, tx_count, byte_count;

		// Read from the file
		byte_count = fread((void *)&packet[3], 1, BLOCK_SIZE_MAX, file);
		if (!byte_count)
			break;

		packet[0] = tar_addr;
		packet[1] = cmd_code;
		packet[2] = byte_count;
		// packet[3:byte_count + 2]
		write_count = byte_count + 2; // (cmd_code & byte_count)

		if (pec) {
			write_count++;
			packet[write_count] = crc8(0, packet, write_count);
		}

		// Write the data to the bus
		tx_count = aa_i2c_write(handle, tar_addr >> 1, AA_I2C_NO_FLAGS,
		                        (u16)write_count, &packet[1]);
		if (tx_count < 0) {
			fprintf(stderr, "error: %s\n", aa_status_string(tx_count));
			goto cleanup;
		} else if (tx_count == 0) {
			fprintf(stderr, "error: no bytes written\n");
			fprintf(stderr, "  are you sure you have the right slave address?\n");
			goto cleanup;
		} else if (tx_count != write_count) {
			fprintf(stderr, "error: only a partial number of bytes written\n");
			fprintf(stderr, "  (%d) instead of full (%d)\n", tx_count, write_count);
			goto cleanup;
		} else
			fprintf(stderr, "wr:%d,tx:%d\n", write_count, tx_count);

		// Dump the data to the screen
		dump_packet(packet, tx_count, "Data written to device:");

		// Sleep a tad to make sure slave has time to process this request
		aa_sleep_ms(10);
	}

cleanup:
	fclose(file);
	return 0;
}
