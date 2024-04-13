#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "aardvark_app.h"
#include "version.h"
#include "types.h"
#include "smbus_core.h"

typedef enum _aa_func_idx_e {
	AA_FUNC_IDX_DETECT = 0,
	AA_FUNC_IDX_SMB_WRITE_FILE = 1,
	AA_FUNC_IDX_I2C_MASTER_WRITE,
	AA_FUNC_IDX_I2C_MASTER_READ,
	AA_FUNC_IDX_I2C_MASTER_WRITE_FILE,
	AA_FUNC_IDX_I2C_SLAVE_POLL,
	AA_FUNC_IDX_SMB_MASTER_BLOCK_READ,
	AA_FUNC_IDX_SMB_DEVICE_POLL,
	AA_FUNC_IDX_TEST,
	AA_FUNC_IDX_TEST2,
	AA_FUNC_IDX_SMB_BLOCK_WRITE,
	AA_FUNC_IDX_SMB_ARP_EXEC,
	AA_FUNC_IDX_MAX
} aa_func_idx_e;

typedef struct _aardvark_func_list_t {
	const char *name;
	aa_func_idx_e index;
} aardvark_func_list_t;

static int m_keep_power = 0;
static u8 block[BLOCK_SIZE_MAX];

static const aardvark_func_list_t aa_func_list[] = {
	{"smb-write-file", AA_FUNC_IDX_SMB_WRITE_FILE},
	{"smb-block-write", AA_FUNC_IDX_SMB_BLOCK_WRITE},
	{"i2c-write-file", AA_FUNC_IDX_I2C_MASTER_WRITE_FILE},
	{"i2c-slave-poll", AA_FUNC_IDX_I2C_SLAVE_POLL},
	{"smb-dev-poll", AA_FUNC_IDX_SMB_DEVICE_POLL},
	{"test-smb-ctrl-tar", AA_FUNC_IDX_TEST},

	{NULL}
};

static void help(int func_sel)
{
	switch (func_sel) {
	case AA_FUNC_IDX_SMB_WRITE_FILE:
		fprintf(stderr,
		        "Usage: aardvark [-a] [-b <bit rate>] [-k] [-c] [-p] [-u] smb-write-file [port]\n"
		        "                [target-address] [cmd-code] [file-name]\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b (bit rate)\n"
		        "    -k (keep target power)\n"
		        "    -c (pec)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n"
		        "  'port' is an integer to indicate a valid port to use\n"
		        "  'target-address' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n"
		        "\nExample 1 (send test.bin to address 0x3a with command 0xf and pec):\n"
		        "  # aardvark -cu smb-write-file 0 0x3a 0xf test.bin\n"
		        "\nExample 2 (send test.bin to address 0x3a with command 0xf and pec, Also, turn\n"
		        "  on the power and keep the power even if the function is complete):\n"
		        "  # aardvark -kcpu smb-write-file 0 0x3a 0xf test.bin\n"
		       );
		break;

	case AA_FUNC_IDX_SMB_BLOCK_WRITE:
		fprintf(stderr,
		        "Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-block-write "
		        "                [port] [target-address] [cmd-code] [<data-byte>...]\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b (bit rate)\n"
		        "    -k (keep target power)\n"
		        "    -c (pec)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n"
		        "  'port' is an integer to indicate a valid port to use\n"
		        "  'target-address' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n"
		        "\nExample 1 (send test.bin to address 0x3a with command 0xf and pec):\n"
		        "  # aardvark -cu smb-write-file 0 0x3a 0xf test.bin\n"
		        "\nExample 2 (send test.bin to address 0x3a with command 0xf and pec, Also, turn\n"
		        "  on the power and keep the power even if the function is complete):\n"
		        "  # aardvark -kcpu smb-write-file 0 0x3a 0xf test.bin\n"
		       );
		break;

	case AA_FUNC_IDX_MAX:
	default:
		fprintf(stderr,
		        "Usage: aardvark [<option(s)>] <command> [<arg(s)>]");
		break;
	}

	exit(1);
}

int parse_cmd_code(const char *cmd_code_opt)
{
	int cmd_code;
	char *end;
	cmd_code = strtol(cmd_code_opt, &end, 0);
	if (*end || cmd_code < 0) {
		fprintf(stderr, "error: invalid command code\n");
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
			fprintf(stderr, "error: invalid bit rate\n");
			return -1;
		}

		return bit_rate;
	}

	return I2C_DEFAULT_BITRATE;
}

/*
 * Parse a CHIP-ADDRESS command line argument and return the corresponding
 * chip address, or a negative value if the address is invalid.
 */
int parse_i2c_address(const char *address_opt, int all_addrs)
{
	long address;
	char *end;
	long min_addr = 0x08;
	long max_addr = 0x77;

	address = strtol(address_opt, &end, 0);
	if (*end || !*address_opt) {
		fprintf(stderr, "error: address is not a number!\n");
		return -1;
	}

	if (all_addrs) {
		min_addr = 0x00;
		max_addr = 0x7f;
	}

	if (address < min_addr || address > max_addr) {
		fprintf(stderr, "error: address out of range "
		        "(valid address is: 0x%02lx-0x%02lx)\n", min_addr, max_addr);
		return -2;
	}

	return address;
}

static void main_exit(int ret, int handle, int func_sel, const char *fmt, ...)
{
	// // Disable the Aardvark adapter's power pins.
	// // This command is only effective on v2.0 hardware or greater.
	// // The power pins on the v1.02 hardware are not enabled by default.
	// if (!m_keep_power)
	//      aa_target_power(handle, AA_TARGET_POWER_NONE);

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

	if (func_sel >= 0)
		help(func_sel);

	exit(ret);
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
	int func_idx = -1;
	int all_addr = 0, pec = 0,  power = 0, pull_up = 0, version = 0, manual = 0;
	int opt, port, real_bit_rate, bit_rate, tar_addr, cmd_code;
	const char *file_name;

	real_bit_rate = bit_rate = I2C_DEFAULT_BITRATE;

	/* handle (optional) flags first */
	while ((opt = getopt(argc, argv, "abchkpuv")) != -1) {
		switch (opt) {
		case 'a': all_addr = 1; break;
		case 'b': bit_rate_opt = optarg; break;
		case 'c': pec = 1; break;
		case 'k': m_keep_power = 1; break;
		case 'p': power = 1; break;
		case 'u': pull_up = 1; break;
		case 'v': version = 1; break;
		case 'h': manual = 1; break;
		}
	}

	if (version)
		main_exit(0, 0, -1, "SMBus Host Software Version %s\n", VERSION);

	if (argc < optind + 1) {
		main_exit(1, 0, AA_FUNC_IDX_MAX, "error: too few arguments\n");
	}

	const char *function = argv[optind];
	for (int i = 0; aa_func_list[i].name != NULL; i++) {
		const char *func_name = aa_func_list[i].name;
		if (strcmp(func_name, function) == 0) {
			func_idx = aa_func_list[i].index;
			break;
		}
	}

	if (func_idx < 0)
		main_exit(1, 0, AA_FUNC_IDX_MAX, "error: no match function\n");

	if (manual)
		main_exit(0, 0, func_idx, NULL);

	if (argc < optind + 2)
		main_exit(1, 0, func_idx, "error: too few arguments\n");

	port = strtol(argv[optind + 1], &end, 0);
	if (*end || port < 0)
		main_exit(1, 0, -1, "error: invalid port number\n");

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		printf("error: unable to open Aardvark device on port %d\n", port);
		printf("Error code = %d\n", handle);
		main_exit(1, 0, -1, NULL);
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_GPIO_I2C);

	// Enable the I2C bus pullup resistors (2.2k resistors).
	// This command is only effective on v2.0 hardware or greater.
	// The pullup resistors on the v1.02 hardware are enabled by default.
	if (pull_up)
		aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);

	// Enable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	if (power)
		aa_target_power(handle, AA_TARGET_POWER_BOTH);

	switch (func_idx) {
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-write-file
	 *                 <port> <target-address> <cmd-code> <file-name>
	 */
	case AA_FUNC_IDX_SMB_WRITE_FILE: {
		if (argc < optind + 5)
			main_exit(1, handle, -1, "error: too few arguments\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		tar_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (tar_addr < 0)
			goto exit;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto exit;

		file_name = argv[optind + 4];
		if (!file_name)
			goto exit;

		// Setup the bit rate
		real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
		if (real_bit_rate != bit_rate)
			fprintf(stderr, "warning: the bitrate is different from user input\n");

		int ret = smbus_write_file(handle, tar_addr, cmd_code, file_name, pec);
		if (ret)
			main_exit(1, handle, -1, "smbus_write_file failed (%d)\n", ret);

		break;
	}
	/**
	 * Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-block-write
	 *                 <port> <target-address> <cmd-code> <data-byte(s)>
	 */
	case AA_FUNC_IDX_SMB_BLOCK_WRITE: {
		if (argc < optind + 5)
			main_exit(1, handle, func_idx, "error: too few arguments\n");

		bit_rate = parse_bit_rate(bit_rate_opt);
		if (bit_rate < 0)
			goto exit;

		tar_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (tar_addr < 0)
			goto exit;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto exit;

		file_name = argv[optind + 4];
		if (!file_name)
			goto exit;

		// Setup the bit rate
		real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
		if (real_bit_rate != bit_rate)
			fprintf(stderr, "warning: the bitrate is different from user input\n");

		if (argc > (int)sizeof(block) + optind + 5)
			main_exit(1, handle, func_idx, "error: too many arguments\n");

		if (argc < optind + 6)
			main_exit(1, handle, func_idx, "error: too few arguments\n");

		int len, value;
		for (len = 0; len + optind + 6 < argc; len++) {
			value = strtol(argv[optind + len + 5], &end, 0);
			if (*end || value < 0)
				main_exit(1, handle, -1, "error: invalid data value '%s'\n",
				          argv[optind + len + 5]);

			if (value > 0xff)
				main_exit(1, handle, -1, "error: data value '%s' out of range\n",
				          argv[optind + len + 5]);

			block[len] = value;
		}

		int ret
		        = smbus_write_block(handle, tar_addr, cmd_code, block, len, pec);
		if (ret)
			main_exit(1, handle, -1, "smbus_write_file failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_DETECT: {
		int ret = aa_detect();
		if (ret)
			printf("error: %s\n", aa_status_string(ret));

		break;
	}
	case AA_FUNC_IDX_I2C_MASTER_WRITE_FILE: {
		if (argc < 5) {
			printf("Usage: aa_i2c_file <port> <tar_addr> <file_name>\n");
			printf("  tar_addr is the target slave address\n");
			printf("\n");
			printf("  'file_name' should contain data to be sent\n");
			printf("  to the downstream i2c device\n");
			return 1;
		}

		int ret = 0;
		int port = atoi(argv[2]);
		u8 tar_addr = (u8)strtol(argv[3], 0, 0);
		char *file_name = argv[4];

		printf("port: %d\n", port);
		printf("target: %02x\n", tar_addr);
		printf("file_name: %s\n", file_name);

		ret = aa_i2c_file(port, tar_addr, file_name);
		if (ret)
			printf("aa_i2c_file failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_I2C_SLAVE_POLL: {
		if (argc < 5) {
			printf("Usage: aai2c_slave <port> <tar_addr> <TIMEOUT_MS>\n");
			printf("  tar_addr is the slave address for this device\n");
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
			printf("aa_i2c_slave_poll failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_SMB_DEVICE_POLL: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 dev_addr = (u8)strtol(argv[3], 0, 0);
		int timeout_ms = atoi(argv[4]);

		printf("port: %d\n", port);
		printf("device: %02x\n", dev_addr);
		printf("timeout(ms): %d\n", timeout_ms);

		ret = aa_smbus_device_poll(port, dev_addr, timeout_ms);
		if (ret)
			printf("aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_TEST: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 tar_addr = (u8)strtol(argv[3], 0, 0);
		u8 dev_addr = (u8)strtol(argv[4], 0, 0);
		int timeout_ms = atoi(argv[5]);

		printf("port: %d\n", port);
		printf("target: %02x,%02x\n", tar_addr, tar_addr >> 1);
		printf("device: %02x,%02x\n", dev_addr, dev_addr >> 1);
		printf("timeout(ms): %d\n", timeout_ms);

		ret = test_smbus_controller_target_poll(port, tar_addr, dev_addr, timeout_ms);
		if (ret)
			printf("aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	default:
		break;
	}

	main_exit(0, handle, -1, NULL);

exit:
	main_exit(1, handle, -1, NULL);
}
