#include "aardvark.h"
#include "smbus.h"
#include "crc.h"
#include "crc8.h"
#include "utility.h"

#include "types.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

const char *smbus_trace_header[TRACE_TYPE_MAX] =  {
	"[SMBus] error: ",
	"[SMBus] warning: ",
	"[SMBus] debug: ",
	"[SMBus] info: ",
	"[SMBus] init: ",
};

static char *smbus_addr_type[] = {
	"DTA",
	"PTA",
	"VTA",
	"RNG"
};

static u8 data[SMBUS_BUF_MAX];

/**
 * @brief Dump the data to the screen
 */
static void dump_packet(const void *buf, int packet_len, const char *fmt, ...)
{
	const char *c = buf;
	if (fmt) {
		va_list argp;
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		// fputc('\n', stderr);
	}

	for (int i = 0; i < packet_len; ++i) {
		if ((i & 0x0f) == 0) {
			fprintf(stderr, "\n%04x:  ", i);
		}
		if (i < 3 || i == (packet_len - 1)) {
			fprintf(stderr, "\033[7m%02x \033[0m", c[i] & 0xff);
		} else {
			fprintf(stderr, "%02x ", c[i] & 0xff);
		}
		if (((i + 1) & 0x07) == 0) {
			fprintf(stderr, " ");
		}
	}
	fprintf(stderr, "\n\n");
}

static int smbus_verify_byte_written(int num_bytes, int num_written)
{
	if (num_written < 0) {
		smbus_trace(ERROR, "%s\n", aa_status_string(num_written));
	} else if (num_written == 0) {
		smbus_trace(ERROR, "no bytes written\n");
		smbus_trace(ERROR, "  are you sure you have the right slave address?\n");
	} else if (num_written != num_bytes) {
		smbus_trace(ERROR, "only a partial number of bytes written\n");
		smbus_trace(ERROR, "  (%d) instead of full (%d)\n", num_written + 1, num_bytes + 1);
	} else {
		// smbus_trace(INFO, "num_bytes:%d, num_written:%d\n", num_bytes + 1, num_written + 1);
		return 0;
	}
	return -1;
}

static int smbus_verify_byte_read(int num_bytes, int num_read)
{
	if (num_read < 0) {
		smbus_trace(ERROR, "%s\n", aa_status_string(num_read));
		return -1;
	} else if (num_read == 0) {
		smbus_trace(ERROR, "no bytes read\n");
		smbus_trace(ERROR, "  are you sure you have the right slave address?\n");
		return -1;
	} else if (num_read != num_bytes) {
		smbus_trace(ERROR, "only a partial number of bytes read\n");
		smbus_trace(ERROR, "  (%d) instead of full (%d)\n", num_read, num_bytes);
		return -1;
	} else {
		// smbus_trace(INFO, "num_bytes:%d, num_read:%d\n", num_bytes, num_read);
		return 0;
	}
}

int smbus_send_byte(Aardvark handle, u8 slv_addr, u8 u8_data, bool pec_flag)
{
	int num_written, num_bytes;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = u8_data;
	num_bytes = 1;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	// Write the data to the bus
	num_written = aa_i2c_write(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1]);
	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write_byte(Aardvark handle, u8 slv_addr, u8 cmd_code, u8 u8_data, bool pec_flag)
{
	int num_written, num_bytes;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = cmd_code;
	data[2] = u8_data;
	num_bytes = 2;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1]);
	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write_word(Aardvark handle, u8 slv_addr, u8 cmd_code, u16 u16_data, bool pec_flag)
{
	int num_written, num_bytes;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = cmd_code;
	data[2] = (u16_data >>  0) & 0xFF;
	data[3] = (u16_data >>  8);
	num_bytes = 3;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1]);
	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write32(Aardvark handle, u8 slv_addr, u8 cmd_code, u32 u32_data, bool pec_flag)
{
	int num_written, num_bytes;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = cmd_code;
	data[2] = (u32_data >>  0) & 0xFF;
	data[3] = (u32_data >>  8) & 0xFF;
	data[4] = (u32_data >> 16) & 0xFF;
	data[5] = (u32_data >> 24);
	num_bytes = 5;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1]);
	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_write64(Aardvark handle, u8 slv_addr, u8 cmd_code, u64 u64_data, bool pec_flag)
{
	int num_written, num_bytes;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = cmd_code;
	data[2] = (u64_data >>  0) & 0xFF;
	data[3] = (u64_data >>  8) & 0xFF;
	data[4] = (u64_data >> 16) & 0xFF;
	data[5] = (u64_data >> 24) & 0xFF;
	data[6] = (u64_data >> 32) & 0xFF;
	data[7] = (u64_data >> 40) & 0xFF;
	data[8] = (u64_data >> 48) & 0xFF;
	data[9] = (u64_data >> 56);
	num_bytes = 9;

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	num_written = aa_i2c_write(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1]);
	if (smbus_verify_byte_written(num_bytes, num_written))
		return -1;

	return 0;
}

int smbus_block_write(Aardvark handle, u8 slv_addr, u8 cmd_code, u8 byte_cnt, const void *buf,
                      u8 pec_flag, int verbose)
{
	int ret, status;
	u16 num_bytes, num_written;

	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = cmd_code;
	data[2] = byte_cnt;
	memcpy(&data[3], buf, byte_cnt);
	num_bytes = byte_cnt + 2; // +2: cmd_code & byte_cnt

	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
		if (pec_flag == 2)
			data[num_bytes] = data[num_bytes] ^ 0xFF;
	}

	status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes, &data[1], &num_written);
	if (status) {
		smbus_trace(ERROR, "aa_i2c_write_ext (%d)\n", status);
		ret = -SMBUS_CMD_WRITE_FAILED;
		goto dump;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		smbus_trace(ERROR, "num written mismatch (%d,%d)\n", num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto dump;
	}

	ret = SMBUS_SUCCESS;

dump:
	if (verbose)
		dump_packet(data, num_bytes + pec_flag, "Data written to device:");
	return ret;
}

int smbus_write_file(Aardvark handle, u8 slv_addr, u8 cmd_code, const char *file_name,
                     bool pec_flag)
{
	int ret, status;
	FILE *file;
	u16 num_bytes = 0, num_written;

	file = fopen(file_name, "rb");
	if (!file) {
		perror("fopen");
		ret = -SMBUS_ERROR;
		goto out;
	}

	while (!feof(file)) {
		u16 byte_cnt;

		// Read from the file
		byte_cnt = fread((void *)&data[3], 1, BLOCK_SIZE_MAX, file);
		if (!byte_cnt)
			break;

		data[0] = slv_addr << 1 | I2C_WRITE;
		data[1] = cmd_code;
		data[2] = byte_cnt;
		// data[3:byte_cnt + 2]
		num_bytes = byte_cnt + 2; // (cmd_code & byte_cnt)

		if (pec_flag) {
			++num_bytes;
			data[num_bytes] = crc8(data, num_bytes);
		}

		// Write the data to the bus
		status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes,
		                          &data[1], &num_written);
		if (status) {
			smbus_trace(ERROR, "aa_i2c_write_ext (%d)\n", status);
			ret = -SMBUS_CMD_WRITE_FAILED;
			goto cleanup;
		}

		if (smbus_verify_byte_written(num_bytes, num_written)) {
			smbus_trace(ERROR, "smbus_verify_byte_written (%d,%d)\n", num_bytes,
			            num_written);
			ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
			goto cleanup;
		}

		dump_packet(data, num_bytes + pec_flag, "Data written to device:");

		// Sleep a tad to make sure slave has time to process this request
		aa_sleep_ms(10);
	}

	ret = SMBUS_SUCCESS;

cleanup:
	fclose(file);
	if (num_bytes)
		dump_packet(data, num_bytes + pec_flag, "Data written to device:");
out:
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

	u8 slv_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = SMBUS_ARP_PREPARE_TO_ARP;
	num_bytes = 1;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}

	status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          &data[1], &num_written);
	if (status) {
		smbus_trace(ERROR, "aa_i2c_write_ext (%d)\n", status);
		ret = -SMBUS_CMD_WRITE_FAILED;
		goto dump;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		smbus_trace(ERROR, "smbus_verify_byte_written (%d,%d)\n", num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto dump;
	}

	ret = SMBUS_SUCCESS;

dump:
	dump_packet(data, num_bytes + pec_flag, "Prepare to ARP");
	return ret;
}

/**
 * @brief This command requests ARP-capable and/or Discoverable devices to
 * return their target address along with their UDID. If directed = 1, then this
 * command requests a specific ARP-capable device to return its Unique Identifier.
 */
int smbus_arp_cmd_get_udid(Aardvark handle, void *udid, u8 tar_addr, bool directed,
                           bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written, num_read;
	u8 pec;

	union smbus_get_udid_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	u8 slv_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slv_addr << 1 | I2C_WRITE;
	if (directed)
		data[1] = tar_addr << 1 | I2C_READ;
	else
		data[1] = SMBUS_ARP_GET_UDID;
	data[2] = slv_addr << 1 | I2C_READ;
	num_bytes = 1;
	status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_STOP, num_bytes,
	                          &data[1], &num_written);
	if (unlikely(status)) {
		smbus_trace(ERROR, "aa_i2c_write_ext (%d)\n", status);
		ret = -SMBUS_CMD_WRITE_FAILED;
		goto dump;
	}
	num_bytes = 19;
	status = aa_i2c_read_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes,
	                         &data[3], &num_read);
	if (unlikely(status & (~AA_I2C_STATUS_SLA_NACK))) {
		smbus_trace(ERROR, "aa_i2c_read_ext (%d)\n", status);
		ret = -SMBUS_CMD_READ_FAILED;
		goto dump;
	}

	if (smbus_verify_byte_read(num_bytes, num_read)) {
		smbus_trace(ERROR, "num read mismatch (%d,%d)\n", num_bytes, num_read);
		ret = -SMBUS_CMD_NUM_READ_MISMATCH;
		goto dump;
	}

	num_bytes = 21;
	if (pec_flag) {
		pec = crc8(data, num_bytes);
		if (p->pec != pec) {
			smbus_trace(ERROR, "[%s]:pec mismatch (%02x,%02x)\n", __func__, p->pec, pec);
			ret = -SMBUS_PEC_ERR;
			goto dump;
		}
	}

	if (p->byte_cnt != 17) {
		smbus_trace(ERROR, "byte count error (%d,%d)\n", p->byte_cnt, 17);
		ret = -SMBUS_CMD_BYTE_CNT_ERR;
		goto dump;
	}

	if (!(p->dev_tar_addr & 1)) {
		smbus_trace(ERROR, "device target address error (%02x)\n", p->dev_tar_addr);
		ret = -SMBUS_CMD_DEV_TAR_ADDR_ERR;
		goto dump;
	}

	memcpy(udid, p->udid, sizeof(p->udid));
	ret = SMBUS_SUCCESS;

dump:
	dump_packet(data, num_bytes + pec_flag, "Get UDID");
	return ret;
}

/**
 * @brief This command forces all non-PTA, ARP-capable devices to return to
 * their initial state. If directed = 1, then this command forces a specific
 * non-PTA, ARP-capable device to return to its initial state.
 */
int smbus_arp_cmd_reset_device(Aardvark handle, u8 tar_addr, u8 directed, bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;
	u8 slv_addr;

	union smbus_reset_device_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	slv_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slv_addr << 1 | I2C_WRITE;
	if (directed)
		data[1] = tar_addr << 1 | I2C_WRITE;
	else
		data[1] = SMBUS_ARP_RESET_DEVICE;
	num_bytes = 1;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}
	status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          &data[1], &num_written);
	if (unlikely(status)) {
		smbus_trace(ERROR, "aa_i2c_write_ext (%d)\n", status);
		ret = -SMBUS_CMD_WRITE_FAILED;
		goto dump;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		smbus_trace(ERROR, "num written mismatch (%d,%d)\n", num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto dump;
	}

	ret = SMBUS_SUCCESS;

dump:
	dump_packet(data, num_bytes + pec_flag, "Reset Device");
	return ret;
}

/**
 * @brief The ARP Controller assigns an address to a specific device with this
 * command.
 */
int smbus_arp_cmd_assign_address(Aardvark handle, const union udid_ds *udid,
                                 u8 dev_tar_addr, bool pec_flag)
{
	int ret, status;
	u16 num_bytes, num_written;

	union smbus_assign_address_ds *p = (void *)data;
	memset(p, 0, sizeof(*p));

	u8 slv_addr = SMBUS_ADDR_DEFAULT;
	data[0] = slv_addr << 1 | I2C_WRITE;
	data[1] = SMBUS_ARP_ASSIGN_ADDRESS;
	data[2] = sizeof(*udid) + 1;
	// Byte[18:3]
	memcpy(&data[3], udid, sizeof(*udid));
	num_bytes = 3 + sizeof(*udid);
	data[num_bytes] = dev_tar_addr << 1 | 1;
	if (pec_flag) {
		++num_bytes;
		data[num_bytes] = crc8(data, num_bytes);
	}
	status = aa_i2c_write_ext(handle, slv_addr, AA_I2C_NO_FLAGS, num_bytes,
	                          &data[1], &num_written);
	if (status) {
		smbus_trace(ERROR, "[%s]:aa_i2c_write_ext failed (%d)\n",
		            __func__, status);
		ret = -SMBUS_CMD_WRITE_FAILED;
		goto dump;
	}

	if (smbus_verify_byte_written(num_bytes, num_written)) {
		smbus_trace(ERROR, "[%s]:num written mismatch (%d,%d)\n", __func__,
		            num_bytes, num_written);
		ret = -SMBUS_CMD_NUM_WRITTEN_MISMATCH;
		goto dump;
	}

	ret = SMBUS_SUCCESS;

dump:
	dump_packet(data, num_bytes + pec_flag, "Assign Address");
	return ret;
}

int smbus_slave_poll_default_callback(const void *buf, u32 len, int verbose)
{
	return 0;
}

int smbus_slave_poll(Aardvark handle, int timeout_ms, bool pec_flag,
                     slave_poll_callback callback, int verbose)
{
	int ret, status;
	int trans_num = 0;

	if (verbose)
		smbus_trace(INFO, "polling smbus data...\n");

	// Polling data from SMBus
	status = aa_async_poll(handle, timeout_ms);
	if (status == AA_ASYNC_NO_DATA) {
		smbus_trace(INFO, "no data available\n");
		ret = -SMBUS_SLV_NO_AVAILABLE_DATA;
		goto out;
	}

	// Loop until aa_async_poll times out
	for (;;) {
		if (status == AA_ASYNC_I2C_READ) {
			u16 num_read;
			u8 slv_addr;
			/**
			 * Read the I2C message.
			 *
			 * This function has an internal timeout (see datasheet), though
			 * since we have already checked for data using aa_async_poll, the
			 * timeout should never be exercised.
			 */
			status = aa_i2c_slave_read_ext(handle, &slv_addr, SMBUS_BUF_MAX,
			                               &data[1], &num_read);
			if (status) {
				smbus_trace(ERROR, "aa_i2c_slave_read_ext (%d)\n", status);
				ret = -SMBUS_SLV_READ_FAILED;
				goto out;
			}

			data[0] = slv_addr << 1 | I2C_WRITE;
			num_read = num_read + 1;

			if (verbose) {
				// Dump the data to the screen
				smbus_trace(INFO, "transaction #%d (%d)\n", trans_num, num_read);
				// print_buf(data, num_read, "data read from smbus:");
				dump_packet(data, num_read, "data read from smbus:");
			}

			if (pec_flag) {
				u8 pec = crc8(data, num_read);
				if (pec != 0) {
					smbus_trace(ERROR, "pec error (%d)\n", pec);
					ret = -SMBUS_PEC_ERR;
					goto out;
				}
			}

			if (callback) {
				status = callback(data, num_read + 1, verbose);
				if (status)
					smbus_trace(WARN, "callback (%d)\n", status);
				// break;
			}
			++trans_num;

		} else if (status == AA_ASYNC_I2C_WRITE) {
			// Get number of bytes written to master
			u16 num_written;
			status = aa_i2c_slave_write_stats_ext(handle, &num_written);
			if (status) {
				smbus_trace(ERROR, "aa_i2c_slave_write_stats_ext (%d)\n", status);
				ret = -SMBUS_SLV_WRITE_FAILED;
				goto out;
			}

			if (verbose) {
				// Print status information to the screen
				smbus_trace(INFO, "transaction #%02d\n", trans_num);
				smbus_trace(INFO, "number of bytes written to smbus master = %d\n",
				            num_written);
			}
		} else if (status == AA_ASYNC_SPI) {
			smbus_trace(ERROR, "non-i2c asynchronous message is pending\n");
			ret = -SMBUS_SLV_RECV_NON_I2C_DATA;
			goto out;
		}

		// Use aa_async_poll to wait for the next transaction
		status = aa_async_poll(handle, timeout_ms);
		if (status == AA_ASYNC_NO_DATA) {
			// If bus idle for more than 60 seconds, just break the loop.
			smbus_trace(INFO, "no more data available from smbus\n");
			break;
		}
	}

	ret = SMBUS_SUCCESS;

out:
	return ret;
}

void print_udid(const union udid_ds *udid)
{
	smbus_trace(INFO, "sizeof(udid_ds) = %d\n", (u32)sizeof(union udid_ds));

	smbus_trace(INFO, "udid->dev_cap.value = %x\n", udid->dev_cap.value);
	smbus_trace(INFO, "PEC Supported = %d\n", udid->dev_cap.pec_sup);

	smbus_trace(INFO, "Address Type = %s (%d)\n", smbus_addr_type[udid->dev_cap.addr_type],
	            udid->dev_cap.addr_type);

	smbus_trace(INFO, "udid->ver_rev.value = %d\n", udid->ver_rev.value);
	smbus_trace(INFO, "Silicon Revision ID = %d\n", udid->ver_rev.si_rev_id);
	smbus_trace(INFO, "UDID Version = %d\n", udid->ver_rev.udid_ver);

	smbus_trace(INFO, "Vendor ID = %04x\n", udid->vendor_id);
	smbus_trace(INFO, "Device ID = %04x\n", udid->device_id);

	smbus_trace(INFO, "udid->interface.value = %x\n", udid->interface.value);
	smbus_trace(INFO, "SMBus Version = %d\n", udid->interface.smbus_ver);
	smbus_trace(INFO, "OEM = %d\n", udid->interface.oem);
	smbus_trace(INFO, "ASF = %d\n", udid->interface.asf);
	smbus_trace(INFO, "IPMI = %d\n", udid->interface.ipmi);
	smbus_trace(INFO, "ZONE = %d\n", udid->interface.zone);

	smbus_trace(INFO, "Subsystem Vendor ID = %04x\n", udid->subsys_vendor_id);
	smbus_trace(INFO, "Subsystem Device ID = %04x\n", udid->subsys_device_id);
	smbus_trace(INFO, "Vendor Specific ID = %08x\n", udid->vendor_spec_id);
}
