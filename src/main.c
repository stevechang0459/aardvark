#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "help.h"

#include "aardvark_app.h"
#include "version.h"
#include "types.h"
#include "smbus.h"
#include "smbus_core.h"

const struct function_list func_list[] = {
	{"detect", FUNC_IDX_DETECT},
	{"smb-send-byte", FUNC_IDX_SMB_SEND_BYTE},
	{"smb-write-byte", FUNC_IDX_SMB_WRITE_BYTE},
	{"smb-write-word", FUNC_IDX_SMB_WRITE_WORD},
	{"smb-write-32", FUNC_IDX_SMB_WRITE_32},
	{"smb-write-64", FUNC_IDX_SMB_WRITE_64},
	{"smb-block-write", FUNC_IDX_SMB_BLOCK_WRITE},
	// SMBus Address Resolution Protocol
	{"prepare-to-arp", FUNC_IDX_SMB_PREPARE_TO_ARP},
	// Application
	{"smb-write-file", FUNC_IDX_SMB_WRITE_FILE},
	{"i2c-write-file", FUNC_IDX_I2C_MASTER_WRITE_FILE},
	{"i2c-slave-poll", FUNC_IDX_I2C_SLAVE_POLL},
	{"smb-dev-poll", FUNC_IDX_SMB_DEVICE_POLL},
	{"test-smb-ctrl-tar", FUNC_IDX_TEST},

	{NULL}
};

static int m_keep_power = 0;
static u8 block[BLOCK_SIZE_MAX];

int parse_cmd_code(const char *cmd_code_opt)
{
	int cmd_code;
	char *end;
	cmd_code = strtol(cmd_code_opt, &end, 0);
	if (*end || cmd_code < 0) {
		fprintf(stderr, "error: invalid command code '%s'\n", cmd_code_opt);
		return -1;
	}

	return cmd_code;
}

int parse_bit_rate(const char *bit_rate_opt)
{
	if (bit_rate_opt) {
		char *end;
		int bit_rate = strtol(bit_rate_opt, &end, 0);
		if (*end || bit_rate <= 0) {
			fprintf(stderr, "error: invalid bit rate '%s'\n", bit_rate_opt);
			return -1;
		}

		return bit_rate;
	}

	return I2C_DEFAULT_BITRATE;
}

/**
 * Parse a CHIP-ADDRESS command line argument and return the corresponding
 * chip address, or a negative value if the address is invalid.
 */
int parse_i2c_address(const char *addr_opt, int all_addrs)
{
	long address;
	char *end;
	long min_addr = 0x08;
	long max_addr = 0x77;

	address = strtol(addr_opt, &end, 0);
	if (*end || !*addr_opt) {
		fprintf(stderr, "error: invalid address '%s'\n", addr_opt);
		return -1;
	}

	if (all_addrs) {
		min_addr = 0x00;
		max_addr = 0x7f;
	}

	if (address < min_addr || address > max_addr) {
		fprintf(stderr, "error: address '%s' out of range "
		        "(valid address is: 0x%02lx-0x%02lx)\n",
		        addr_opt, min_addr, max_addr);
		return -2;
	}

	return address;
}

static void main_exit(int status_code, int handle, int func_idx,
                      const char *fmt, ...)
{
	/**
	 * Disable the Aardvark adapter's power pins. This command is only effective
	 * on v2.0 hardware or greater. The power pins on the v1.02 hardware are not
	 * enabled by default.
	 */
	if (!m_keep_power)
		aa_target_power(handle, AA_TARGET_POWER_NONE);

	if (fmt) {
		va_list argp;
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		fputc('\n', stderr);
	}

	// Close the device
	if (handle)
		aa_close(handle);

	if (func_idx > FUNC_IDX_NULL)
		help(func_idx);

	exit(status_code);
}

int main(int argc, char *argv[])
{
#if (OPT_ARDVARK_TRACE)
	printf("argc:%d\n", argc);
	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];
		printf("%d,%s\n", strlen(arg), arg);
	}
#endif

	Aardvark handle = 0;
	char *end, *bit_rate_opt = NULL;
	int func_idx = FUNC_IDX_NULL;
	int all_addr = 0, pec = 0,  power = 0, pull_up = 0, version = 0, manual = 0;
	int opt, port, real_bit_rate, bit_rate, slv_addr, cmd_code,
	    i2c_slave_mode;
	const char *file_name;

	real_bit_rate = bit_rate = I2C_DEFAULT_BITRATE;

	/* handle (optional) flags first */
	while ((opt = getopt(argc, argv, "ab:chkpsuv")) != -1) {
		switch (opt) {
		case 'a':
			all_addr = 1;
			break;
		case 'b':
			bit_rate_opt = optarg;
			break;
		case 'c':
			pec = 1;
			break;
		case 'k':
			m_keep_power = 1;
			break;
		case 'p':
			power = 1;
			break;
		case 's':
			i2c_slave_mode = 1;
			break;
		case 'u':
			pull_up = 1;
			break;
		case 'v':
			version = 1;
			break;
		case 'h':
			manual = 1;
			break;
		case '?':
			main_exit(opt == '?', 0, FUNC_IDX_MAX, NULL);
		}
	}

	if (version)
		main_exit(EXIT_SUCCESS, 0, -1, "SMBus Host Software Version %s\n",
		          VERSION);

	if (argc < optind + 1) {
		if (manual)
			main_exit(EXIT_SUCCESS, 0, FUNC_IDX_MAX, NULL);
		else
			main_exit(EXIT_FAILURE, 0, FUNC_IDX_MAX,
			          "error: too few arguments\n");
	}

	// argc == optind + 1
	const char *f = argv[optind];
	for (int i = 0; func_list[i].name != NULL; i++) {
		const char *func_name = func_list[i].name;
		if (strcmp(func_name, f) == 0) {
			func_idx = func_list[i].func_idx;
			break;
		}
	}

	if (func_idx < 0)
		main_exit(EXIT_FAILURE, 0, FUNC_IDX_MAX, "error: no match function\n");

	if (manual)
		main_exit(EXIT_SUCCESS, 0, func_idx, NULL);

	if (argc < optind + 2)
		main_exit(EXIT_FAILURE, 0, func_idx, "error: too few arguments\n");

	// argc == optind + 2
	port = strtol(argv[optind + 1], &end, 0);
	if (*end || port < 0)
		main_exit(EXIT_FAILURE, 0, -1, "error: invalid port number\n");

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		fprintf(stderr,
		        "error: unable to open Aardvark device on port %d\n", port);
		fprintf(stderr, "Error code = %d\n", handle);
		main_exit(EXIT_FAILURE, 0, -1, NULL);
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_GPIO_I2C);

	/**
	 * Enable the I2C bus pullup resistors (2.2k resistors). This command is
	 * only effective on v2.0 hardware or greater. The pullup resistors on the
	 * v1.02 hardware are enabled by default.
	 */
	if (pull_up)
		aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);

	/**
	 * Enable the Aardvark adapter's power pins. This command is only effective
	 * on v2.0 hardware or greater. The power pins on the v1.02 hardware are not
	 * enabled by default.
	 */
	if (power)
		aa_target_power(handle, AA_TARGET_POWER_BOTH);

	if (i2c_slave_mode)
		aa_i2c_slave_enable(handle, SMBUS_ADDR_NVME_MI_BMC, 0, 0);

	switch (func_idx) {
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-send-byte
	 *                 <port> <slv_addr> <data>
	 */
	case FUNC_IDX_SMB_SEND_BYTE: {
		if (argc < optind + 4)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too few arguments\n");
		else if (argc > optind + 4)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too many arguments\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto exit;

		uint32_t data = strtoul(argv[optind + 3], &end, 0);
		if (*end || errno) {
			perror("strtoul");
			main_exit(EXIT_FAILURE, handle, -1,
			          "error: invalid data value '%s'\n",
			          argv[optind + 3]);
		}

		if (data > UINT8_MAX)
			main_exit(EXIT_FAILURE, handle, -1,
			          "error: data value '%s' out of range\n",
			          argv[optind + 3]);

		int ret = smbus_send_byte(handle, slv_addr, data, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-write-xxx
	 *                 <port> <slv_addr> <cmd_code> <data>
	 */
	case FUNC_IDX_SMB_WRITE_BYTE:
	case FUNC_IDX_SMB_WRITE_WORD:
	case FUNC_IDX_SMB_WRITE_32:
	case FUNC_IDX_SMB_WRITE_64: {
		if (argc < optind + 5)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too few arguments\n");
		else if (argc > optind + 5)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too many arguments\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto exit;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto exit;

		uint64_t max = 0;
		switch (func_idx) {
		case FUNC_IDX_SMB_WRITE_BYTE:
			max = UINT8_MAX;
			break;
		case FUNC_IDX_SMB_WRITE_WORD:
			max = UINT16_MAX;
			break;
		case FUNC_IDX_SMB_WRITE_32:
			max = UINT32_MAX;
			break;
		}

		uint64_t data = strtoull(argv[optind + 4], &end, 0);
		if (*end || errno) {
			perror("strtoull");
			main_exit(EXIT_FAILURE, handle, -1,
			          "error: invalid data value '%s'\n",
			          argv[optind + 4]);
		}

		if (max && (data > max))
			main_exit(EXIT_FAILURE, handle, -1,
			          "error: data value '%s' out of range\n",
			          argv[optind + 4]);

		int ret;
		switch (func_idx) {
		case FUNC_IDX_SMB_WRITE_BYTE:
			ret = smbus_write_byte(
			              handle, slv_addr, cmd_code, (u8)data, pec);
			break;
		case FUNC_IDX_SMB_WRITE_WORD:
			ret = smbus_write_word(
			              handle, slv_addr, cmd_code, (u16)data, pec);
			break;
		case FUNC_IDX_SMB_WRITE_32:
			ret = smbus_write32(handle, slv_addr, cmd_code, (u32)data, pec);
			break;
		case FUNC_IDX_SMB_WRITE_64:
			ret = smbus_write64(handle, slv_addr, cmd_code, data, pec);
			break;
		}

		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-block-write
	 *                 <port> <slv_addr> <cmd_code> <data>
	 */
	case FUNC_IDX_SMB_BLOCK_WRITE: {
		if (argc < optind + 5)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too few arguments\n");
		else if (argc > optind + 4 + sizeof(block))
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too many arguments\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto exit;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto exit;

		// Setup the bit rate
		real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
		if (real_bit_rate != bit_rate)
			fprintf(stderr,
			        "warning: the bitrate is different from user input\n");

		int byte_cnt, value;
		for (byte_cnt = 0; byte_cnt + optind + 4 < argc; byte_cnt++) {
			value = strtol(argv[byte_cnt + optind + 4], &end, 0);
			if (*end || value < 0)
				main_exit(EXIT_FAILURE, handle, -1,
				          "error: invalid data value '%s'\n",
				          argv[byte_cnt + optind + 5]);

			if (value > 0xff)
				main_exit(EXIT_FAILURE, handle, -1,
				          "error: data value '%s' out of range\n",
				          argv[byte_cnt + optind + 5]);

			block[byte_cnt] = value;
		}

		int ret = smbus_block_write(handle, slv_addr, cmd_code, byte_cnt,
		                            block, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_SMB_PREPARE_TO_ARP: {
		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		// Setup the bit rate
		real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
		if (real_bit_rate != bit_rate)
			fprintf(stderr,
			        "warning: the bitrate is different from user input\n");

		int ret = smbus_arp_cmd_prepare_to_arp(handle, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-write-file
	 *                 <port> <slv_addr> <cmd_code> <file_name>
	 */
	case FUNC_IDX_SMB_WRITE_FILE: {
		if (argc < optind + 5)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too few arguments\n");
		else if (argc > optind + 5)
			main_exit(EXIT_FAILURE, handle, func_idx,
			          "error: too namy argumentds\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto exit;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto exit;

		file_name = argv[optind + 4];

		// Setup the bit rate
		real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
		if (real_bit_rate != bit_rate)
			fprintf(stderr,
			        "warning: the bitrate is different from user input\n");

		int ret = smbus_write_file(
		                  handle, slv_addr, cmd_code, file_name, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_DETECT: {
		int ret = aa_detect();
		if (ret)
			fprintf(stderr, "error: %s\n", aa_status_string(ret));

		break;
	}
	case FUNC_IDX_I2C_MASTER_WRITE_FILE: {
		if (argc < 5) {
			printf("Usage: aa_i2c_file <port> <slv_addr> <file_name>\n");
			printf("  'slv_addr' is the target slave address\n");
			printf("\n");
			printf("  'file_name' should contain data to be sent\n");
			printf("  to the downstream i2c device\n");
			return 1;
		}

		int ret = 0;
		int port = atoi(argv[2]);
		u8 slv_addr = (u8)strtol(argv[3], 0, 0);
		char *file_name = argv[4];

		printf("port: %d\n", port);
		printf("target: %02x\n", slv_addr);
		printf("file_name: %s\n", file_name);

		ret = aa_i2c_file(port, slv_addr, file_name);
		if (ret)
			fprintf(stderr, "aa_i2c_file failed (%d)\n", ret);

		break;
	}
	case FUNC_IDX_I2C_SLAVE_POLL: {
		if (argc < 5) {
			printf("Usage: aai2c_slave <port> <slv_addr> <timeout_ms>\n");
			printf("  'slv_addr' is the slave address for this device\n");
			printf("\n");
			printf("  The timeout value specifies the time to\n");
			printf("  block until the first packet is received.\n");
			printf("  If the timeout is -1, the program will\n");
			printf("  block indefinitely.\n");
			return 1;
		}

		int ret = 0;
		int port = atoi(argv[2]);
		u8 dev_addr = (u8)strtol(argv[3], 0, 0);
		int timeout_ms = atoi(argv[4]);

		printf("port: %d\n", port);
		printf("device: %02x\n", dev_addr);
		printf("timeout(ms): %d\n", timeout_ms);

		ret = aa_i2c_slave_poll(port, dev_addr, timeout_ms);
		if (ret)
			fprintf(stderr, "aa_i2c_slave_poll failed (%d)\n", ret);

		break;
	}
	case FUNC_IDX_SMB_DEVICE_POLL: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 dev_addr = (u8)strtol(argv[3], 0, 0);
		int timeout_ms = atoi(argv[4]);

		printf("port: %d\n", port);
		printf("device: %02x\n", dev_addr);
		printf("timeout(ms): %d\n", timeout_ms);

		ret = aa_smbus_device_poll(port, dev_addr, timeout_ms);
		if (ret)
			fprintf(stderr, "aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	case FUNC_IDX_TEST: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 slv_addr = (u8)strtol(argv[3], 0, 0);
		u8 dev_addr = (u8)strtol(argv[4], 0, 0);
		int timeout_ms = atoi(argv[5]);

		printf("port: %d\n", port);
		printf("target: %02x,%02x\n", slv_addr, slv_addr >> 1);
		printf("device: %02x,%02x\n", dev_addr, dev_addr >> 1);
		printf("timeout(ms): %d\n", timeout_ms);

		ret = test_smbus_controller_target_poll(
		              port, slv_addr, dev_addr, timeout_ms);
		if (ret)
			fprintf(stderr, "aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	default:
		break;
	}

	main_exit(EXIT_SUCCESS, handle, -1, NULL);

exit:
	main_exit(EXIT_FAILURE, handle, -1, NULL);
}
