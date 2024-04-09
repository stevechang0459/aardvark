#include "global.h"

#include "aardvark_app.h"

typedef enum _aa_func_idx_e {
	AA_FUNC_IDX_DETECT = 0,
	AA_FUNC_IDX_OPEN = 1,
	AA_FUNC_IDX_CLOSE,
	AA_FUNC_IDX_I2C_MASTER_WRITE,
	AA_FUNC_IDX_I2C_MASTER_READ,
	AA_FUNC_IDX_I2C_MASTER_WRITE_FILE,
	AA_FUNC_IDX_I2C_SLAVE_POLL,
	AA_FUNC_IDX_SMB_MASTER_BLOCK_WRITE,
	AA_FUNC_IDX_SMB_MASTER_BLOCK_READ,
	AA_FUNC_IDX_SMB_SLAVE_POLL,
	AA_FUNC_IDX_TEST,

	AA_FUNC_IDX_MAX
} aa_func_idx_e;

typedef struct _aardvark_func_list_t {
	const char *name;
	aa_func_idx_e index;
} aardvark_func_list_t;

static const aardvark_func_list_t aa_func_list[] = {
	{"detect", AA_FUNC_IDX_DETECT},
	{"open", AA_FUNC_IDX_OPEN},
	{"close", AA_FUNC_IDX_CLOSE},
	{"i2c-write", AA_FUNC_IDX_I2C_MASTER_WRITE},
	{"i2c-read", AA_FUNC_IDX_I2C_MASTER_READ},
	{"i2c-slave-poll", AA_FUNC_IDX_I2C_SLAVE_POLL},
	{"i2c-write-file", AA_FUNC_IDX_I2C_MASTER_WRITE_FILE},
	{"smb-block-write", AA_FUNC_IDX_SMB_MASTER_BLOCK_WRITE},
	{"smb-block-read", AA_FUNC_IDX_SMB_MASTER_BLOCK_READ},
	{"smb-slave-poll", AA_FUNC_IDX_SMB_SLAVE_POLL},
	{"test-smb-ctrl-tgt", AA_FUNC_IDX_TEST},

	{NULL}
};

static void die(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fputc('\n', stderr);
	exit(1);
}

#define OPT_ARDVARK_TRACE (1)

static int interaction_mode = 0;

int main(int argc, char *argv[])
{
	const char *arg;

#if (OPT_ARDVARK_TRACE)
	printf("argc:%d\n", argc);
	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];
		printf("%d,%s\n", strlen(arg), arg);
	}
#endif

	if (argc < 2) {
		die("bad arguments: aardvark [-I] <function> [option(s)] <value> ...");
	}

	// Extract flag
	while ((arg = argv[1]) != NULL) {
		if (*arg != '-')
			break;
		for (;;) {
			switch (*++arg) {
			case '0':
				break;
			case 'I':
				interaction_mode = 1;
				continue;
			default:
				die("Unknown flag '%s'", arg);
			}
			break;
		}
		argv++;
	}

	const char *function = argv[1];
	int func_idx = -1;

	for (int i = 0; aa_func_list[i].name != NULL; i++) {
		const char *func_name = aa_func_list[i].name;
		printf("%s,%s\n", func_name, function);
		if (strcmp(func_name, function) == 0) {
			func_idx = aa_func_list[i].index;
			break;
		}
	}

	switch (func_idx) {
	case AA_FUNC_IDX_DETECT: {
		int ret = aa_detect();
		printf("aa_detect: %d\n", ret);
		printf("aa_status_string: %s\n", aa_status_string(ret));

		break;
	}
	case AA_FUNC_IDX_OPEN: {
		Aardvark handle;
		int port = atoi(argv[2]);

		// Open the device
		handle = aa_open(port);
		if (handle <= 0) {
			printf("aa_open: Unable to open Aardvark device on port %d\n",
			       port);
			printf("Error code = %d\n", handle);
			return 1;
		} else {
			printf("aa_open success\n");
		}

		// Disable the Aardvark adapter's power pins.
		// This command is only effective on v2.0 hardware or greater.
		// The power pins on the v1.02 hardware are not enabled by default.
		aa_target_power(handle, AA_TARGET_POWER_BOTH);
		break;
	}
	case AA_FUNC_IDX_CLOSE: {
		Aardvark handle;
		int port = atoi(argv[2]);

		// Open the device
		handle = aa_open(port);
		if (handle <= 0) {
			printf("Unable to open Aardvark device on port %d\n", port);
			printf("Error code = %d\n", handle);
			return 1;
		}

		// Disable the Aardvark adapter's power pins.
		// This command is only effective on v2.0 hardware or greater.
		// The power pins on the v1.02 hardware are not enabled by default.
		aa_target_power(handle, AA_TARGET_POWER_NONE);

		// Disable the slave and close the device
		aa_i2c_slave_disable(handle);

		int dev_cnt = aa_close(handle);
		if (!dev_cnt)
			printf("aa_close failed\n");
		else
			printf("aa_close success (%d)\n", dev_cnt);

		break;
	}
	case AA_FUNC_IDX_I2C_MASTER_WRITE_FILE: {
		if (argc < 5) {
			printf("usage: aa_i2c_file <PORT> <SLAVE_ADDR> <filename>\n");
			printf("  SLAVE_ADDR is the target slave address\n");
			printf("\n");
			printf("  'filename' should contain data to be sent\n");
			printf("  to the downstream i2c device\n");
			return 1;
		}

		int ret = 0;
		int port = atoi(argv[2]);
		u8 addr = (u8)strtol(argv[3], 0, 0);
		char *filename = argv[4];

		printf("port: %d\n", port);
		printf("addr: %02x\n", addr);
		printf("filename: %s\n", filename);

		ret = aa_i2c_file(port, addr, filename);
		if (ret)
			printf("aa_i2c_file failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_I2C_SLAVE_POLL: {
		if (argc < 4) {
			printf("usage: aai2c_slave <PORT> <SLAVE_ADDR> <TIMEOUT_MS>\n");
			printf("  SLAVE_ADDR is the slave address for this device\n");
			printf("\n");
			printf("  The timeout value specifies the time to\n");
			printf("  block until the first packet is received.\n");
			printf("  If the timeout is -1, the program will\n");
			printf("  block indefinitely.\n");
			return 1;
		}

		int ret = 0;
		int port = atoi(argv[2]);
		u8 addr = (u8)strtol(argv[3], 0, 0);
		int timeout_ms = atoi(argv[4]);

		printf("port: %d\n", port);
		printf("addr: %02x\n", addr);
		printf("timeout_ms: %d\n", timeout_ms);

		ret = aa_i2c_slave_poll(port, addr, timeout_ms);
		if (ret)
			printf("aa_i2c_slave_poll failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_I2C_MASTER_WRITE:
		break;

	case AA_FUNC_IDX_I2C_MASTER_READ:
		break;

	case AA_FUNC_IDX_SMB_MASTER_BLOCK_WRITE:
		break;

	case AA_FUNC_IDX_SMB_MASTER_BLOCK_READ:
		break;

	case AA_FUNC_IDX_SMB_SLAVE_POLL: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 addr = (u8)strtol(argv[3], 0, 0);
		int timeout_ms = atoi(argv[4]);

		printf("port: %d\n", port);
		printf("addr: %02x\n", addr);
		printf("timeout_ms: %d\n", timeout_ms);

		ret = aa_smb_slave_poll(port, addr, timeout_ms);
		if (ret)
			printf("aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	case AA_FUNC_IDX_TEST: {
		int ret = 0;
		int port = atoi(argv[2]);
		u8 tgt_addr = (u8)strtol(argv[3], 0, 0);
		u8 ctrl_addr = (u8)strtol(argv[4], 0, 0);
		int timeout_ms = atoi(argv[5]);

		printf("port: %d\n", port);
		printf("tgt_addr: %02x\n", tgt_addr);
		printf("ctrl_addr: %02x\n", ctrl_addr);
		printf("timeout_ms: %d\n", timeout_ms);

		ret = test_smb_controller_target_poll(port, tgt_addr, ctrl_addr, timeout_ms);
		if (ret)
			printf("aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	default:
		die("bad arguments: aardvark <function> [option] <value>");
	}

	return 0;
}
