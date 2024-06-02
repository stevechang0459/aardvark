#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "types.h"
#include "aardvark.h"
#include "smbus.h"
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
			fprintf(stderr, "\n%04x:  ", i);
		}
		fprintf(stderr, "%02x ", c[i + 1] & 0xff);
		if (((i + 1) & 0x07) == 0) {
			fprintf(stderr, " ");
		}
	}
	fprintf(stderr, "\n\n");
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
		fprintf(stderr, "  (%d) instead of full (%d)\n", num_written,
		        num_bytes);
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

int smbus_send_byte(Aardvark handle, u8 slave_addr, u8 u8_data, bool pec_flag)
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
                     bool pec_flag)
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
                     bool pec_flag)
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
                  bool pec_flag)
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
                  bool pec_flag)
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
                      u8 byte_cnt, const void *buf, bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;
	data[0] = slave_addr;
	data[1] = cmd_code;
	data[2] = byte_cnt;
	memcpy(&data[3], buf, byte_cnt);
	num_bytes = byte_cnt + 2; // (cmd_code & byte_cnt)

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	// Write the data to the bus
	status = aa_i2c_write_ext(handle, slave_addr >> 1, AA_I2C_NO_FLAGS,
	                          num_bytes, &data[1], &num_written);
	if (status) {
		fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n",
		        __FUNCTION__, status);
		return -SMBUS_CMD_WRITE_FAILED;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		fprintf(stderr, "[%s]:num written mismatch (%d,%d)\n",
		        __FUNCTION__, num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto finish;
	}

	ret = SMBUS_CMD_SUCCESS;

finish:
	// Dump the data to the screen
	dump_packet(data, num_bytes + pec_flag, "Data written to device:");

	return ret;
}

int smbus_write_file(Aardvark handle, u8 slave_addr, u8 cmd_code,
                     const char *file_name, bool pec_flag)
{
	int ret, status;
	// Open the file
	FILE *file = fopen(file_name, "rb");
	if (!file) {
		perror("fopen");
		return -1;
	}

	while (!feof(file)) {
		u16 num_bytes, num_written, byte_cnt;

		// Read from the file
		byte_cnt = fread((void *)&data[3], 1, BLOCK_SIZE_MAX, file);
		if (!byte_cnt)
			break;

		data[0] = slave_addr << 1;
		data[1] = cmd_code;
		data[2] = byte_cnt;
		// data[3:byte_cnt + 2]
		num_bytes = byte_cnt + 2; // (cmd_code & byte_cnt)

		if (pec_flag) {
			++num_bytes;
			data[num_bytes] = crc8(data, num_bytes);
		}

		// Write the data to the bus
		status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_FLAGS,
		                          num_bytes, &data[1], &num_written);
		if (status) {
			fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n",
			        __FUNCTION__, status);
			ret = -SMBUS_CMD_WRITE_FAILED;
			goto cleanup;
		}

		if (smbus_verify_byte_written(num_bytes, num_written)) {
			fprintf(stderr, "[%s]:num written mismatch (%d,%d)\n",
			        __FUNCTION__, num_bytes, num_written);
			ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
			goto cleanup;
		}

		// Dump the data to the screen
		dump_packet(data, num_bytes + pec_flag, "Data written to device:");

		// Sleep a tad to make sure slave has time to process this request
		aa_sleep_ms(10);
	}

	ret = SMBUS_CMD_SUCCESS;

cleanup:
	fclose(file);
	return ret;
}

/**
 * @brief This command informs all devices that the ARP Controller is starting
 * the ARP process.
 */
int smbus_arp_cmd_prepare_to_arp(Aardvark handle, bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;

	union smbus_prepare_to_arp_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

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
		fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n",
		        __FUNCTION__, status);
		return -SMBUS_CMD_WRITE_FAILED;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		fprintf(stderr, "[%s]:num written mismatch (%d,%d)\n",
		        __FUNCTION__, num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto finish;
	}

	ret = SMBUS_CMD_SUCCESS;

finish:
	dump_packet(data, num_bytes + pec_flag, "Prepare to ARP");

	return ret;
}

/**
 * @brief This command requests ARP-capable and/or Discoverable devices to
 * return their target address along with their UDID. If directed = 1, then this
 * command requests a specific ARP-capable device to return its Unique Identifier.
 */
int smbus_arp_cmd_get_udid(Aardvark handle, void *udid, u8 tar_addr,
                           bool directed, bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written, num_read;
	u8 pec;

	union smbus_get_udid_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	u8 slave_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slave_addr << 1;
	if (directed)
		data[1] = tar_addr << 1 | 1;
	else
		data[1] = SMBUS_ARP_GET_UDID;
	data[2] = slave_addr << 1 | I2C_READ;
	num_bytes = 1;
	status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_STOP, num_bytes,
	                          &data[1], &num_written);
	if (unlikely(status)) {
		fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n",
		        __FUNCTION__, status);
		return -SMBUS_CMD_WRITE_FAILED;
	}
	num_bytes = 18;
	status = aa_i2c_read_ext(handle, slave_addr, AA_I2C_NO_FLAGS, num_bytes,
	                         &data[3], &num_read);
	if (unlikely(status)) {
		fprintf(stderr, "[%s]:aa_i2c_read_ext failed (%d)\n",
		        __FUNCTION__, status);
		return -SMBUS_CMD_READ_FAILED;
	}

	if (smbus_verify_byte_read(num_bytes, num_read)) {
		fprintf(stderr, "[%s]:num read mismatch (%d,%d)\n",
		        __FUNCTION__, num_bytes, num_read);
		ret = -SMBUS_CMD_NUM_READ_MISMATCH;
		goto finish;
	}

	num_bytes = 21;
	if (pec_flag) {
		pec = crc8(data, num_bytes);
		if (p->pec != pec) {
			fprintf(stderr, "[%s]:pec mismatch (%02x,%02x)\n",
			        __FUNCTION__, p->pec, pec);
			ret = -SMBUS_CMD_PEC_ERR;
			goto finish;
		}
	}

	if (p->byte_cnt != 17) {
		fprintf(stderr, "[%s]:byte count error (%02x,%02x)\n",
		        __FUNCTION__, p->byte_cnt, 17);
		ret = SMBUS_CMD_BYTE_CNT_ERR;
		goto finish;
	}

	if (!(p->dev_tar_addr & 1)) {
		fprintf(stderr, "[%s]:device target address error (%02x)\n",
		        __FUNCTION__, p->dev_tar_addr);
		ret = -SMBUS_CMD_DEV_TAR_ADDR_ERR;
		goto finish;
	}

	memcpy(udid, p->udid, sizeof(p->udid));
	ret = SMBUS_CMD_SUCCESS;

finish:
	dump_packet(data, num_bytes + pec_flag, "Get UDID");

	return ret;
}

/**
 * @brief This command forces all non-PTA, ARP-capable devices to return to
 * their initial state. If directed = 1, then this command forces a specific
 * non-PTA, ARP-capable device to return to its initial state.
 */
int smbus_arp_cmd_reset_device(Aardvark handle, u8 tar_addr, u8 directed,
                               bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;

	union smbus_reset_device_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	u8 slave_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slave_addr << 1;
	if (directed)
		data[1] = tar_addr << 1;
	else
		data[1] = SMBUS_ARP_RESET_DEVICE;
	num_bytes = 1;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}
	status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          data, &num_written);
	if (unlikely(status)) {
		fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n", __FUNCTION__,
		        status);
		return -SMBUS_CMD_WRITE_FAILED;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		fprintf(stderr, "[%s]:num written mismatch (%d,%d)\n",
		        __FUNCTION__, num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto finish;
	}

	ret = SMBUS_CMD_SUCCESS;

finish:
	dump_packet(data, num_bytes + pec_flag, "Reset Device");

	return ret;
}

/**
 * @brief The ARP Controller assigns an address to a specific device with this
 * command.
 */
int smbus_arp_cmd_assign_address(Aardvark handle, const void *udid, u8 tar_addr,
                                 bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;

	union smbus_assign_address_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	u8 slave_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slave_addr << 1;
	data[1] = SMBUS_ARP_ASSIGN_ADDRESS;
	data[2] = 17;
	// Byte[18:3]
	memcpy(&data[3], udid, 16);
	data[19] = tar_addr;
	num_bytes = 19;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}
	status = aa_i2c_write_ext(handle, slave_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          &data[1], &num_written);
	if (status) {
		fprintf(stderr, "[%s]:aa_i2c_write_ext failed (%d)\n",
		        __FUNCTION__, status);
		return -SMBUS_CMD_WRITE_FAILED;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		fprintf(stderr, "[%s]:num written mismatch (%d,%d)\n",
		        __FUNCTION__, num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto finish;
	}

	ret = SMBUS_CMD_SUCCESS;

finish:
	dump_packet(data, num_bytes + pec_flag, "Assign Address");

	return ret;
}
