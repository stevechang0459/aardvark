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
	AA_FUNC_IDX_SMB_WRITE_FILE,
	AA_FUNC_IDX_I2C_MASTER_WRITE,
	AA_FUNC_IDX_I2C_MASTER_READ,
	AA_FUNC_IDX_I2C_MASTER_WRITE_FILE,
	AA_FUNC_IDX_I2C_SLAVE_POLL,
	AA_FUNC_IDX_SMB_MASTER_BLOCK_READ,
	AA_FUNC_IDX_SMB_DEVICE_POLL,
	AA_FUNC_IDX_TEST,
	AA_FUNC_IDX_TEST2,
	AA_FUNC_IDX_SMB_WRITE_FILE,
	AA_FUNC_IDX_MAX
} aa_func_idx_e;

typedef struct _aardvark_func_list_t {
	const char *name;
	aa_func_idx_e index;
} aardvark_func_list_t;

static const aardvark_func_list_t aa_func_list[] = {
	{"smb-write-file", AA_FUNC_IDX_SMB_WRITE_FILE},
	{"detect", AA_FUNC_IDX_DETECT},
	{"open", AA_FUNC_IDX_OPEN},
	{"close", AA_FUNC_IDX_CLOSE},
	{"i2c-write", AA_FUNC_IDX_I2C_MASTER_WRITE},
	{"i2c-read", AA_FUNC_IDX_I2C_MASTER_READ},
	{"i2c-slave-poll", AA_FUNC_IDX_I2C_SLAVE_POLL},
	{"i2c-write-file", AA_FUNC_IDX_I2C_MASTER_WRITE_FILE},
	{"smb-block-read", AA_FUNC_IDX_SMB_MASTER_BLOCK_READ},
	{"smb-dev-poll", AA_FUNC_IDX_SMB_DEVICE_POLL},
	{"test-smb-ctrl-tar", AA_FUNC_IDX_TEST},
	{"test-smb-block-write", AA_FUNC_IDX_TEST2},

	{NULL}
};

static void help(aa_func_idx_e func_sel)
{
	switch (func_sel) {
	case AA_FUNC_IDX_SMB_WRITE_FILE:
		fprintf(stderr,
		        "Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-write-file <port> <target-address> <cmd-code> <file-name>\n"
		        "  I2CBUS is an integer or an I2C bus name\n"
		        "  ADDRESS is an integer (0x08 - 0x77, or 0x00 - 0x7f if -a is given)\n"
		        "  MODE is one of:\n"
		        "    c (byte, no value)\n"
		        "    b (byte data, default)\n"
		        "    w (word data)\n"
		        "    i (I2C block data)\n"
		        "    s (SMBus block data)\n"
		        "    Append p for SMBus PEC\n");
		break;

	case AA_FUNC_IDX_SMB_ARP_EXEC:
		fprintf(stderr,
		        "Usage: aardvark [-k] [-c] [-p] [-u] [-b <bitrate>] [-d <device-address>] arp-exec\n"
		        "I2CBUS DESC [DATA] [DESC [DATA]]...\n"
		        "  I2CBUS is an integer or an I2C bus name\n"
		        "  DESC describes the transfer in the form: {r|w}LENGTH[@address]\n"
		        "    1) read/write-flag 2) LENGTH (range 0-65535, or '?')\n"
		        "    3) I2C address (use last one if omitted)\n"
		        "  DATA are LENGTH bytes for a write message. They can be shortened by a suffix:\n"
		        "    = (keep value constant until LENGTH)\n"
		        "    + (increase value by 1 until LENGTH)\n"
		        "    - (decrease value by 1 until LENGTH)\n"
		        "    p (use pseudo random generator until LENGTH with value as seed)\n\n"
		        "Example (bus 0, read 8 byte at offset 0x64 from EEPROM at 0x50):\n"
		        "  # i2ctransfer 0 w1@0x50 0x64 r8\n"
		        "Example (same EEPROM, at offset 0x42 write 0xff 0xfe ... 0xf0):\n"
		        "  # i2ctransfer 0 w17@0x50 0x42 0xff-\n");
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
		fprintf(stderr, "Error: Command code invalid\n");
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
			fprintf(stderr, "Error: Bit rate invalid\n");
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
		fprintf(stderr, "Error: Address is not a number!\n");
		return -1;
	}

	if (all_addrs) {
		min_addr = 0x00;
		max_addr = 0x7f;
	}

	if (address < min_addr || address > max_addr) {
		fprintf(stderr, "Error: Address out of range "
		        "(valid address is: 0x%02lx-0x%02lx)\n", min_addr, max_addr);
		return -2;
	}

	return address;
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

	Aardvark handle;
	char *end, *bit_rate_opt = NULL;
	int version = 0, power = 0, keep_power = 0, pull_up = 0, all_addr = 0;
	int pec = 0, opt;
	int port, real_bit_rate, bit_rate, tar_addr, cmd_code;
	const char *file_name;

	real_bit_rate = bit_rate = I2C_DEFAULT_BITRATE;

	/* handle (optional) flags first */
	while ((opt = getopt(argc, argv, "abchkpuv")) != -1) {
		switch (opt) {
		case 'a': all_addr = 1; break;
		case 'b': bit_rate_opt = optarg; break;
		case 'c': pec = 1; break;
		case 'k': keep_power = 1; break;
		case 'p': power = 1; break;
		case 'u': pull_up = 1; break;
		case 'v': version = 1; break;
		case 'h':
		case '?':
			help(AA_FUNC_IDX_MAX);
			exit(opt == '?');
		}
	}

	if (version) {
		fprintf(stderr, "SMBus Host Software Version %s\n", VERSION);
		exit(0);
	}

	printf("optind=%d,argc=%d\n", optind, argc);
	if (argc == optind) {
		help(AA_FUNC_IDX_MAX);
		exit(0);
	}

	const char *function = argv[optind];
	int func_idx = -1;

	for (int i = 0; aa_func_list[i].name != NULL; i++) {
		const char *func_name = aa_func_list[i].name;
		printf("%s,%s\n", func_name, function);
		if (strcmp(func_name, function) == 0) {
			func_idx = aa_func_list[i].index;
			break;
		}
	}

	if (argc < optind + 2) {
		help(AA_FUNC_IDX_MAX);
		exit(0);
	}

	port = strtol(argv[optind + 1], &end, 0);
	if (*end || port < 0)
		fprintf(stderr, "Error: Port invalid!\n");

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		printf("Error: Unable to open Aardvark device on port %d (%x)\n", port,
		       port);
		printf("Error code = %d\n", handle);
		exit(1);
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
	// Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] smb-write-file
	//                 <port> <target-address> <cmd-code> <file-name>
	case AA_FUNC_IDX_SMB_WRITE_FILE: {
		int ret;
		if (argc < optind + 5) {
			help(AA_FUNC_IDX_SMB_WRITE_FILE);
			goto exit;
		}

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
			printf("Warning: The bitrate is different from user input\n");

		ret = smbus_write_file(handle, tar_addr, file_name, pec);
		if (ret) {
			printf("smbus_write_file failed (%d)\n", ret);
			goto exit;
		}

		break;
	}
	case AA_FUNC_IDX_DETECT: {
		int ret = aa_detect();
		if (ret)
			printf("Error: %s\n", aa_status_string(ret));

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
	case AA_FUNC_IDX_I2C_MASTER_WRITE:
		break;

	case AA_FUNC_IDX_I2C_MASTER_READ:
		break;

	case AA_FUNC_IDX_SMB_MASTER_BLOCK_READ:
		break;

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

	// Disable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	if (!keep_power)
		aa_target_power(handle, AA_TARGET_POWER_NONE);

	// Close the device
	aa_close(handle);
	exit(0);

exit:
	// Disable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	if (!keep_power)
		aa_target_power(handle, AA_TARGET_POWER_NONE);

	// Close the device
	aa_close(handle);
	exit(1);
}
