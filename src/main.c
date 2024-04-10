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
	AA_FUNC_IDX_SMB_DEVICE_POLL,
	AA_FUNC_IDX_TEST,
	AA_FUNC_IDX_TEST2,

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
	{"smb-dev-poll", AA_FUNC_IDX_SMB_DEVICE_POLL},
	{"test-smb-ctrl-tar", AA_FUNC_IDX_TEST},
	{"test-smb-block-write", AA_FUNC_IDX_TEST2},

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

int main(int argc, char *argv[])
{
#if (OPT_ARDVARK_TRACE)
	printf("argc:%d\n", argc);
	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];
		printf("%d,%s\n", strlen(arg), arg);
	}
#endif

	if (argc < 2) {
		die("Bad arguments: aardvark [-I] <function> [option(s)] <value> ...");
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

	case AA_FUNC_IDX_SMB_MASTER_BLOCK_WRITE:
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
		if (argc != 6)
			die("Bad arguments (%d): aardvark test-smb-ctrl-tgt <port> <target address> <device address> <timeout>", argc);

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
	case AA_FUNC_IDX_TEST2: {
		if (argc != 5)
			die("Bad arguments (%d): aardvark test-smb-block-write <port> <target address> <file name>", argc);

		int ret = 0;
		int port = atoi(argv[2]);
		u8 tar_addr = (u8)strtol(argv[3], 0, 0);
		char *file_name = argv[4];

		printf("port: %d\n", port);
		printf("target address: %02x,%02x\n", tar_addr, tar_addr >> 1);
		printf("file name: %s\n", file_name);

		ret = test_smbus_file_block_write(port, tar_addr, file_name);
		if (ret)
			printf("aa_smb_slave_poll failed (%d)\n", ret);

		break;
	}
	default:
		die("Bad arguments: aardvark <function> [option(s)] <value(s)>");
	}

	return 0;
}
