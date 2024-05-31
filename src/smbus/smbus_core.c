#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "smbus.h"
#include "smbus_core.h"
#include "aardvark.h"
#include "types.h"
#include "crc.h"

static u8 data[SMBUS_BUF_MAX];

static void dump_packet(const void *buf, int packet_len, const char *fmt, ...)
{
	const char *c = buf;
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
		printf("%02x ", c[i + 1] & 0xff);
		if (((i + 1) & 0x07) == 0) {
			printf(" ");
		}
	}
	printf("\n\n");
}

static int smbus_verify_byte_written(int num_bytes, int num_written)
{
	if (num_written < 0) {
		fprintf(stderr, "error: %s\n", aa_status_string(num_written));
		return -1;
	} else if (num_written == 0) {
		fprintf(stderr, "error: no bytes written\n");
		fprintf(stderr, "  are you sure you have the right slave address?\n");
		return -1;
	} else if (num_written != num_bytes) {
		fprintf(stderr, "error: only a partial number of bytes written\n");
		fprintf(stderr, "  (%d) instead of full (%d)\n", num_written, num_bytes);
		return -1;
	} else {
		// fprintf(stderr, "wr:%d,tx:%d\n", num_bytes, num_written);
		return 0;
	}
}

static int smbus_verify_byte_read(int num_bytes, int num_read)
{
	if (num_read < 0) {
		fprintf(stderr, "error: %s\n", aa_status_string(num_read));
		return -1;
	} else if (num_read == 0) {
		fprintf(stderr, "error: no bytes read\n");
		fprintf(stderr, "  are you sure you have the right slave address?\n");
		return -1;
	} else if (num_read != num_bytes) {
		fprintf(stderr, "error: only a partial number of bytes read\n");
		fprintf(stderr, "  (%d) instead of full (%d)\n", num_read, num_bytes);
		return -1;
	} else {
		// fprintf(stderr, "wr:%d,tx:%d\n", num_bytes, num_read);
		return 0;
	}
}

int smbus_send_byte(Aardvark handle, u8 slave_addr, u8 u8_data, u8 pec_flag)
{
	int num_written, num_bytes;
	data[0] = slave_addr << 1;
	data[1] = u8_data;
	num_bytes = 1;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	// Write the data to the bus
	num_written = aa_i2c_write(handle, slave_addr, AA_I2C_NO_FLAGS,
	                           num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write_byte(Aardvark handle, u8 slave_addr, u8 cmd_code, u8 u8_data,
                     u8 pec_flag)
{
	int num_written, num_bytes;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = u8_data;
	num_bytes = 2;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                           num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write_word(Aardvark handle, u8 slave_addr, u8 cmd_code, u16 u16_data,
                     u8 pec_flag)
{
	int num_written, num_bytes;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = u16_data & 0xFF;
	data[3] = u16_data >> 8;
	num_bytes = 3;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                           num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write32(Aardvark handle, u8 slave_addr, u8 cmd_code, u32 u32_data,
                  u8 pec_flag)
{
	int num_written, num_bytes;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = u32_data & 0xFF;
	data[3] = (u32_data >> 8) & 0xFF;
	data[4] = (u32_data >> 16) & 0xFF;
	data[5] = u32_data >> 24;
	num_bytes = 5;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                           num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write64(Aardvark handle, u8 slave_addr, u8 cmd_code, u64 u64_data,
                  u8 pec_flag)
{
	int num_written, num_bytes;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = u64_data & 0xFF;
	data[3] = (u64_data >> 8) & 0xFF;
	data[4] = (u64_data >> 16) & 0xFF;
	data[5] = (u64_data >> 24) & 0xFF;
	data[6] = (u64_data >> 32) & 0xFF;
	data[7] = (u64_data >> 40) & 0xFF;
	data[8] = (u64_data >> 48) & 0xFF;
	data[9] = u64_data >> 56;
	num_bytes = 9;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                           num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_block_write(Aardvark handle, u8 slave_addr, u8 cmd_code,
                      const u8 *block, u8 byte_count, u8 pec_flag)
{
	int num_bytes, num_written;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = byte_count;
	memcpy((void *)&data[3], block, byte_count);
	num_bytes = byte_count + 2; // (cmd_code & byte_count)

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	// Write the data to the bus
	num_written = aa_i2c_write(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                           (u16)num_bytes, &data[1]);

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	// Dump the data to the screen
	dump_packet(data, num_written, "Data written to device:");

	return 0;
}

int smbus_write_file(Aardvark handle, u8 slave_addr, u8 cmd_code,
                     const char *file_name, u8 pec_flag)
{
	// Open the file
	FILE *file = fopen(file_name, "rb");
	if (!file) {
		perror("fopen");
		return -1;
	}

	while (!feof(file)) {
		int num_bytes, num_written, byte_count;

		// Read from the file
		byte_count = fread((void *)&data[3], 1, BLOCK_SIZE_MAX, file);
		if (!byte_count)
			break;

		data[0] = slave_addr << 1;
		data[1] = cmd_code;
		data[2] = byte_count;
		// data[3:byte_count + 2]
		num_bytes = byte_count + 2; // (cmd_code & byte_count)

		if (pec_flag) {
			++num_bytes;
			data[num_bytes] = crc8(data, num_bytes);
		}

		// Write the data to the bus
		num_written = aa_i2c_write(handle, slave_addr, AA_I2C_NO_FLAGS,
		                           (u16)num_bytes, &data[1]);

		if (smbus_verify_byte_written(num_bytes, num_written))
			goto cleanup;

		// Dump the data to the screen
		dump_packet(data, num_written, "Data written to device:");

		// Sleep a tad to make sure slave has time to process this request
		aa_sleep_ms(10);
	}

cleanup:
	fclose(file);
	return 0;
}

int smbus_arp_cmd_prepare_to_arp(Aardvark handle, bool pec_flag)
{
	int status;
	u16 num_bytes, num_written;

	u8 slave_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slave_addr << 1;
	data[1] = SMBUS_ARP_PREPARE_TO_ARP;
	num_bytes = 1;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          &data[1], &num_written);
	if (status) {
		printf("[%s]:aa_i2c_write_ext failed (%d)\n", __FUNCTION__, status);
		return -1;
	}

	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	dump_packet(data, num_written, "Prepare to ARP");

	return 0;
}

int smbus_arp_cmd_get_udid(Aardvark handle, bool pec_flag)
{
	int status;
	u16 num_bytes, num_written, num_read;
	u8 pec;

	u8 slave_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slave_addr << 1;
	data[1] = SMBUS_ARP_GET_UDID;
	num_bytes = 1;
	status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_STOP, num_bytes,
	                          &data[1], &num_written);
	if (status) {
		printf("[%s]:aa_i2c_write_ext failed (%d)\n", __FUNCTION__, status);
		return -1;
	}
	num_bytes = 18;
	status = aa_i2c_read_ext(handle, slave_addr, AA_I2C_NO_FLAGS, num_bytes,
	                         &data[3], &num_read);
	if (status) {
		printf("[%s]:aa_i2c_read_ext failed (%d)\n", __FUNCTION__, status);
		return -1;
	}

	if (smbus_verify_byte_read(num_bytes, num_read))
		return -1;

	data[2] = slave_addr << 1 | I2C_READ;
	if (pec_flag) {
		pec = crc8(data, 21);
		union smbus_get_udid *p = (void *)data;
		if (pec != p->pec) {
			printf("[%s]:pec mismatch (%02x,%02x)\n", __FUNCTION__, p->pec,
			       pec);
			return -1;
		}
	}

	dump_packet(data, num_written, "Get UDID");

	return 0;
}
