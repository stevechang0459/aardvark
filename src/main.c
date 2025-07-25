#include "main.h"
#include "help.h"
#include "version.h"
#include "utility.h"

#include "aardvark_app.h"

#include "smbus.h"
#include "mctp.h"
#include "mctp_core.h"
#include "mctp_transport.h"
#include "mctp_message.h"

#include "nvme_cmd.h"
#include "nvme/nvme.h"
#include "libnvme_types.h"
#include "libnvme_mi_mi.h"
#include "nvme_mi.h"

#include "global.h"
#include "types.h"

#include "i2c.h"

#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>
#if (CONFIG_AA_MULTI_THREAD)
#include <pthread.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

extern char *optarg;
extern int optind;

const struct function_list func_list[] = {
	{"detect",            FUNC_IDX_DETECT},
	{"smb-send-byte",     FUNC_IDX_SMB_SEND_BYTE},
	{"smb-write-byte",    FUNC_IDX_SMB_WRITE_BYTE},
	{"smb-write-word",    FUNC_IDX_SMB_WRITE_WORD},
	{"smb-write-32",      FUNC_IDX_SMB_WRITE_32},
	{"smb-write-64",      FUNC_IDX_SMB_WRITE_64},
	{"smb-block-write",   FUNC_IDX_SMB_BLOCK_WRITE},
	// SMBus Address Resolution Protocol
	{"prepare-to-arp",    FUNC_IDX_SMB_PREPARE_TO_ARP},
	{"get-udid",          FUNC_IDX_SMB_GET_UDID},
	{"reset-device",      FUNC_IDX_SMB_RESET_DEVICE},
	{"assign-address",    FUNC_IDX_SMB_ASSIGN_ADDR},
	// Application
	{"smb-write-file",    FUNC_IDX_SMB_WRITE_FILE},
	{"test-mctp",         FUNC_IDX_TEST_MCTP},
	{"smb-slv-poll",      FUNC_IDX_SMB_DEVICE_POLL},
	{"i2cdetect",         FUNC_IDX_I2C_DETECT},
	// {"i2c-write-file",    FUNC_IDX_I2C_MASTER_WRITE_FILE},
	// {"i2c-slave-poll",    FUNC_IDX_I2C_SLAVE_POLL},
	// {"test-smb-ctrl-tar", FUNC_IDX_TEST},

	{NULL}
};

const char *main_trace_header[TRACE_TYPE_MAX] =  {
	"[aardvark] error: ",
	"[aardvark] warning: ",
	"[aardvark] debug: ",
	"[aardvark] info: ",
	"[aardvark] init: ",
};

static int m_keep_power = 0;
static u8 block[BLOCK_SIZE_MAX];

int parse_eid(const char *eid_opt)
{
	int eid;
	char *end;
	eid = strtol(eid_opt, &end, 0);
	if (*end || eid < 0) {
		main_trace(ERROR, "invalid eid '%s'\n", eid_opt);
		return -1;
	}

	if ((eid != 0) && (eid < 0x08 || eid > 0xfe)) {
		main_trace(ERROR, "eid '%s' out of range (valid eid is: 0x00, 0x08-0x7e)\n", eid_opt);
		return -1;
	}

	return eid;
}

int parse_cmd_code(const char *cmd_code_opt)
{
	int cmd_code;
	char *end;
	cmd_code = strtol(cmd_code_opt, &end, 0);
	if (*end || cmd_code < 0) {
		main_trace(ERROR, "invalid command code '%s'\n", cmd_code_opt);
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
			main_trace(ERROR, "invalid bit rate '%s'\n", bit_rate_opt);
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

	if (!addr_opt) {
		main_trace(ERROR, "null addr_opt\n");
		return -1;
	}

	address = strtol(addr_opt, &end, 0);
	if (*end || !*addr_opt) {
		main_trace(ERROR, "invalid address '%s'\n", addr_opt);
		return -1;
	}

	if (all_addrs) {
		min_addr = 0x00;
		max_addr = 0x7f;
	}

	if (address < min_addr || address > max_addr) {
		main_trace(ERROR, "address '%s' out of range (valid address is: 0x%02lx-0x%02lx)\n",
		           addr_opt, min_addr, max_addr);
		return -1;
	}

	return address;
}

static int check_argc_range(int argc, int min, int max)
{
	if (argc < min) {
		main_trace(ERROR, "too few arguments\n");
		return argc - min;
	} else if (argc > max) {
		main_trace(ERROR, "too many arguments\n");
		return argc - max;
	}

	return 0;
}

static void main_exit(int status_code, int handle, int func_idx, const char *fmt, ...)
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

	mctp_deinit();

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
	char *end, *bit_rate_opt = NULL, *host_addr_opt = NULL;
	int func_idx = FUNC_IDX_NULL;
	int all_addr = 0, pec = 0,  power = 0, pull_up = 0, version = 0, manual = 0,
	    directed = 0, i2c_slave_mode = 0, wrong_pec = 0, verbose = 0;
	int opt, port, real_bit_rate, bit_rate, slv_addr, cmd_code;
	const char *file_name;

	real_bit_rate = bit_rate = I2C_DEFAULT_BITRATE;

	/* handle (optional) flags first */
	while ((opt = getopt(argc, argv, "ab:cdhkps:uvV")) != -1) {
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
		case 'd':
			directed = 1;
			break;
		case 'k':
			m_keep_power = 1;
			break;
		case 'p':
			power = 1;
			break;
		case 's':
			host_addr_opt = optarg;
			i2c_slave_mode = 1;
			break;
		case 'u':
			pull_up = 1;
			break;
		case 'v':
			version = 1;
			break;
		case 'V':
			verbose = 1;
			break;
		case 'w':
			wrong_pec = 1;
			break;
		case 'h':
			manual = 1;
			break;
		case '?':
			main_exit(opt == '?', 0, FUNC_IDX_MAX, NULL);
		}
	}

	if (version)
		main_exit(EXIT_SUCCESS, 0, -1, "SMBus Host Software Version %s\n", VERSION);

	if (argc < optind + 1) {
		if (manual)
			main_exit(EXIT_SUCCESS, 0, FUNC_IDX_MAX, NULL);
		else
			main_exit(EXIT_FAILURE, 0, FUNC_IDX_MAX, "error: too few arguments\n");
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
		main_trace(ERROR, "unable to open Aardvark device on port %d\n", port);
		main_trace(ERROR, "Error code = %d\n", handle);
		main_exit(EXIT_FAILURE, 0, -1, NULL);
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_GPIO_I2C);

	bit_rate = parse_bit_rate(bit_rate_opt);
	if (bit_rate < 0)
		goto out;

	// Setup the bit rate
	real_bit_rate = aa_i2c_bitrate(handle, bit_rate);
	if (real_bit_rate != bit_rate)
		main_trace(WARN, "the bitrate is different from user input\n");

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

	// if (i2c_slave_mode)
	//      aa_i2c_slave_enable(handle, SMBUS_ADDR_NVME_MI_BMC, 0, 0);

	switch (func_idx) {
	case FUNC_IDX_DETECT: {
		int ret = aa_detect();
		if (ret)
			main_trace(ERROR, "%s\n", aa_status_string(ret));

		break;
	}
	case FUNC_IDX_SMB_SEND_BYTE: {
		if (check_argc_range(argc, optind + 4, optind + 4))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		uint32_t data = strtoul(argv[optind + 3], &end, 0);
		if (*end || errno == -ERANGE) {
			perror("strtoul");
			main_exit(EXIT_FAILURE, handle, -1, "error: invalid data value '%s'\n",
			          argv[optind + 3]);
		}

		if (data > UINT8_MAX)
			main_exit(EXIT_FAILURE, handle, -1, "error: data value '%s' out of range\n",
			          argv[optind + 3]);

		int ret = smbus_send_byte(handle, slv_addr, data, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_SMB_WRITE_BYTE:
	case FUNC_IDX_SMB_WRITE_WORD:
	case FUNC_IDX_SMB_WRITE_32:
	case FUNC_IDX_SMB_WRITE_64: {
		if (check_argc_range(argc, optind + 5, optind + 5))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto out;

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
		if (*end || errno == -EINVAL || errno == -ERANGE) {
			perror("strtoull");
			main_exit(EXIT_FAILURE, handle, -1, "error: invalid data value '%s'\n",
			          argv[optind + 4]);
		}

		if (max && (data > max))
			main_exit(EXIT_FAILURE, handle, -1, "error: data value '%s' out of range\n",
			          argv[optind + 4]);

		int ret;
		switch (func_idx) {
		case FUNC_IDX_SMB_WRITE_BYTE:
			ret = smbus_write_byte(handle, slv_addr, cmd_code, (u8)data, pec);
			break;
		case FUNC_IDX_SMB_WRITE_WORD:
			ret = smbus_write_word(handle, slv_addr, cmd_code, (u16)data, pec);
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
	case FUNC_IDX_SMB_BLOCK_WRITE: {
		if (check_argc_range(argc, optind + 5, optind + 4 + sizeof(block)))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto out;

		int byte_cnt, value;
		for (byte_cnt = 0; byte_cnt < argc - (optind + 4); byte_cnt++) {
			value = strtol(argv[byte_cnt + optind + 4], &end, 0);
			if (*end || value < 0)
				main_exit(EXIT_FAILURE, handle, -1, "error: invalid data value '%s'\n",
				          argv[byte_cnt + optind + 4]);

			if (value > 0xff)
				main_exit(EXIT_FAILURE, handle, -1, "error: data value '%s' out of range\n",
				          argv[byte_cnt + optind + 4]);

			block[byte_cnt] = value;
		}

		pec = !wrong_pec ? pec : 2;
		int ret = smbus_block_write(handle, slv_addr, cmd_code, byte_cnt,
		                            block, pec, verbose);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_SMB_PREPARE_TO_ARP: {
		if (check_argc_range(argc, optind + 2, optind + 2))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		int ret = smbus_arp_cmd_prepare_to_arp(handle, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_SMB_GET_UDID: {
		slv_addr = 0;
		if (directed) {
			if (check_argc_range(argc, optind + 3, optind + 3))
				main_exit(EXIT_FAILURE, handle, func_idx, NULL);

			slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
			if (slv_addr < 0)
				goto out;
		} else {
			if (check_argc_range(argc, optind + 2, optind + 2))
				main_exit(EXIT_FAILURE, handle, func_idx, NULL);
		}

		union udid_ds udid;
		int ret = smbus_arp_cmd_get_udid(handle, &udid, slv_addr, directed, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		reverse(&udid, sizeof(udid));
		print_udid(&udid);
		break;
	}
	case FUNC_IDX_SMB_RESET_DEVICE: {
		slv_addr = 0;
		if (directed) {
			if (check_argc_range(argc, optind + 3, optind + 3))
				main_exit(EXIT_FAILURE, handle, func_idx, NULL);

			slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
			if (slv_addr < 0)
				goto out;
		} else {
			if (check_argc_range(argc, optind + 2, optind + 2))
				main_exit(EXIT_FAILURE, handle, func_idx, NULL);
		}

		int ret = smbus_arp_cmd_reset_device(handle, slv_addr, directed, pec);
		if (ret)
			main_exit(EXIT_FAILURE, handle, -1, NULL);

		break;
	}
	case FUNC_IDX_SMB_ASSIGN_ADDR: {
		union udid_ds udid;
		if (check_argc_range(argc, optind + 3 + sizeof(udid), optind + 3 + sizeof(udid)))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		int byte_cnt, value;
		for (byte_cnt = 0; byte_cnt < argc - (optind + 3); byte_cnt++) {
			value = strtol(argv[byte_cnt + optind + 2], &end, 0);
			if (*end || value < 0) {
				main_trace(ERROR, "invalid data value '%s'\n", argv[byte_cnt + optind + 2]);
				goto out;
			}

			if (value > 0xff) {
				main_trace(ERROR, "data value '%s' out of range\n", argv[byte_cnt + optind + 2]);
				goto out;
			}

			udid.data[byte_cnt] = value;
		}

		slv_addr = parse_i2c_address(argv[byte_cnt + optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		int ret = smbus_arp_cmd_assign_address(handle, &udid, slv_addr, pec);
		if (ret) {
			main_trace(ERROR, "smbus_arp_cmd_assign_address (%d)\n", ret);
			goto out;
		}

		break;
	}
	case FUNC_IDX_SMB_WRITE_FILE: {
		if (check_argc_range(argc, optind + 5, optind + 5))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		cmd_code = parse_cmd_code(argv[optind + 3]);
		if (cmd_code < 0)
			goto out;

		file_name = argv[optind + 4];

		int ret = smbus_write_file(handle, slv_addr, cmd_code, file_name, pec);
		if (ret) {
			main_trace(ERROR, "smbus_write_file (%d)\n", ret);
			goto out;
		}

		break;
	}
	case FUNC_IDX_TEST_MCTP: {
		int ret;
		union udid_ds udid;
		u8 host_addr;
		int owner_eid, tar_eid;

		if (check_argc_range(argc, optind + 5, optind + 5))
			main_exit(EXIT_FAILURE, handle, func_idx, NULL);

		if (i2c_slave_mode) {
			host_addr = parse_i2c_address(host_addr_opt, all_addr);
			if (host_addr < 0)
				goto out;
		} else
			host_addr = SMBUS_ADDR_IPMI_BMC;

		aa_i2c_slave_enable(handle, host_addr, 0, 0);

		slv_addr = parse_i2c_address(argv[optind + 2], all_addr);
		if (slv_addr < 0)
			goto out;

		owner_eid = parse_eid(argv[optind + 3]);
		if (owner_eid < 0 || owner_eid < 8) {
			main_trace(ERROR, "wrong owner_eid (%d)\n", owner_eid);
			goto out;
		}

		tar_eid = parse_eid(argv[optind + 4]);
		if (tar_eid < 0 || tar_eid < 8 || owner_eid == tar_eid) {
			main_trace(ERROR, "wrong tar_eid (%d,%d)\n", owner_eid, tar_eid);
			goto out;
		}

		main_trace(INFO, "eid (%d,%d)\n", owner_eid, tar_eid);
		ret = smbus_arp_cmd_prepare_to_arp(handle, pec);
		if (ret) {
			main_trace(ERROR, "smbus_arp_cmd_prepare_to_arp (%d)\n", ret);
			goto out;
		}

		ret = smbus_arp_cmd_get_udid(handle, &udid, slv_addr, 0, pec);
		if (ret) {
			main_trace(ERROR, "smbus_arp_cmd_get_udid (%d)\n", ret);
			goto out;
		}

		reverse(&udid, sizeof(udid));
		print_udid(&udid);
		reverse(&udid, sizeof(udid));

		ret = smbus_arp_cmd_assign_address(handle, &udid, slv_addr, pec);
		if (ret) {
			main_trace(ERROR, "smbus_arp_cmd_assign_address (%d)\n", ret);
			goto out;
		}

		ret = smbus_arp_cmd_get_udid(handle, &udid, slv_addr, 1, pec);
		if (ret) {
			main_trace(ERROR, "smbus_arp_cmd_get_udid (%d)\n", ret);
			goto out;
		}

		reverse(&udid, sizeof(udid));
		print_udid(&udid);

		// owner_eid = (owner_eid == 0 ? 8 : owner_eid);
		// tar_eid = (owner_eid == 0xfe ? owner_eid - 1 : owner_eid + 1);

		ret = mctp_init(handle, owner_eid, tar_eid, SMBUS_ADDR_IPMI_BMC,
		                MCTP_BASELINE_TRAN_UNIT_SIZE, pec);
		if (ret) {
			main_trace(ERROR, "mctp_init (%d)\n", ret);
			goto out;
		}

		ret = mctp_message_set_eid(slv_addr, EID_NULL_DST, SET_EID, tar_eid, 1, 0, verbose);
		if (ret) {
			main_trace(ERROR, "mctp_message_set_eid (%d)\n", ret);
			goto out;
		}

		ret = smbus_slave_poll(handle, 100, pec, mctp_receive_packet_handle, verbose);
		if (ret && ret != 0xFF) {
			main_trace(ERROR, "smbus_slave_poll (%d)\n", ret);
			goto out;
		}

#if (!CONFIG_AA_MULTI_THREAD)
		struct aa_args args = {
			.handle = handle,
			.verbose = verbose,
			.slv_addr = slv_addr,
			.dst_eid = tar_eid,
			.csi = 0,
			.nsid = NVME_NSID_ALL,
			.pec = pec,
			.ic = true,
			.timeout = 100,
		};
		int count = 0;
		while (1) {
			args.csi = 0;

			union temp_thresh
			{
				struct
				{
					uint32_t tmpth  : 16;
					uint32_t tmpsel : 4;
					uint32_t thsel  : 2;
					uint32_t rsvd   : 10;
				};
				uint32_t value;
			};

			union temp_thresh tt = {
				.value = 0x137
			};
			args.csi = !args.csi;
			ret = nvme_set_features_temp_thresh(&args, tt.value, 0);
			if (ret) {
				main_trace(ERROR, "nvme_set_features_temp_thresh (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_get_features_temp_thresh(&args, NVME_GET_FEATURES_SEL_CURRENT);
			if (ret) {
				main_trace(ERROR, "nvme_get_features_temp_thresh (%d)\n", ret);
				goto out;
			}
#if 1
			args.csi = !args.csi;
			ret = nvme_get_features_power_mgmt(&args, NVME_GET_FEATURES_SEL_CURRENT);
			if (ret) {
				main_trace(ERROR, "nvme_get_features_power_mgmt (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_identify_ctrl(&args);
			if (ret) {
				main_trace(ERROR, "nvme_identify_ctrl (%d)\n", ret);
				goto out;
			}
#endif
			args.csi = !args.csi;
			ret = nvme_get_log_smart(&args, NVME_NSID_ALL, true);
			if (ret) {
				main_trace(ERROR, "nvme_get_log_smart (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_subsystem_health_status_poll(&args, true);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_subsystem_health_status_poll (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_controller_health_status_poll(&args, true);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_controller_health_status_poll (%d)\n", ret);
				goto out;
			}

			tt.value = 0x189;
			args.csi = !args.csi;
			ret = nvme_set_features_temp_thresh(&args, tt.value, 0);
			if (ret) {
				main_trace(ERROR, "nvme_set_features_temp_thresh (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_get_features_temp_thresh(&args, NVME_GET_FEATURES_SEL_CURRENT);
			if (ret) {
				main_trace(ERROR, "nvme_get_features_temp_thresh (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_get_log_smart(&args, NVME_NSID_ALL, true);
			if (ret) {
				main_trace(ERROR, "nvme_get_log_smart (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_subsystem_health_status_poll(&args, true);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_subsystem_health_status_poll (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_controller_health_status_poll(&args, true);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_controller_health_status_poll (%d)\n", ret);
				goto out;
			}
#if 1
			args.csi = !args.csi;
			union nvme_mi_nmd0 nmd0 = {.value = 0,};
			union nvme_mi_nmd1 nmd1 = {.value = 0,};
			ret = nvme_mi_mi_config_get(&args, nmd0, nmd1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_get_sif (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_set(&args, nmd0, nmd1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_get_sif (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_get_sif(&args);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_get_sif (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_get_mtus(&args);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_get_mtus (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_get_hsc(&args);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_get_hsc (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_set_sif(&args, NVME_MI_PORT_ID_SMBUS, SMBUS_FREQ_400KHZ);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_set_sif (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_set_sif(&args, NVME_MI_PORT_ID_PCIE, SMBUS_FREQ_100KHZ);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_set_sif (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			union nmd1_config_hsc hsc = {
				.value = 0xFFFFFFFF,
			};
			ret = nvme_mi_mi_config_set_hsc(&args, hsc);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_set_hsc (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			union nmd1_config_mtus mtus = {
				.value = 0xFFFFFFFF,
			};
			ret = nvme_mi_mi_config_set_mtus(&args, NVME_MI_PORT_ID_SMBUS, mtus);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_set_mtus (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_config_set_mtus(&args, NVME_MI_PORT_ID_PCIE, mtus);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_config_set_mtus (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_nvm_subsys_info(&args);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_nvm_subsys_info (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_port_info(&args, NVME_MI_PORT_ID_PCIE);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_port_info (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_port_info(&args, NVME_MI_PORT_ID_SMBUS);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_port_info (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_ctrl_list(&args, 0);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_ctrl_list (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_ctrl_list(&args, 1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_ctrl_list (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_ctrl_info(&args, 0);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_ctrl_info (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_ctrl_info(&args, 1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_ctrl_info (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_opt_cmd_support(&args, 0, 0);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_opt_cmd_support (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_opt_cmd_support(&args, 1, 0);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_opt_cmd_support (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_opt_cmd_support(&args, 0, 1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_opt_cmd_support (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_data_read_opt_cmd_support(&args, 1, 1);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_data_read_opt_cmd_support (%d)\n", ret);
				goto out;
			}

			char vpd[256];
			memset(vpd, 0, sizeof(vpd));

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_read(&args, 0, 256, vpd);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_read (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_read(&args, 1, 256, vpd);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_read (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_read(&args, 0, 257, vpd);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_read (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_write(&args, 0, 128, vpd);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_write (%d)\n", ret);
				goto out;
			}

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_write(&args, 128, 128, vpd + 128);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_write (%d)\n", ret);
				goto out;
			}

			memset(vpd, 0, sizeof(vpd));

			args.csi = !args.csi;
			ret = nvme_mi_mi_vpd_read(&args, 0, 256, vpd);
			if (ret) {
				main_trace(ERROR, "nvme_mi_mi_vpd_read (%d)\n", ret);
				goto out;
			}
#endif
			printf("Round #%d done\n", ++count);
#ifdef WIN32
			Sleep(1000);
#else
			usleep(1000 * 1000);
#endif
		}
#else
		pthread_t t1;
		struct aa_args t1_args = {
			.handle = handle,
			.verbose = verbose,
			.slv_addr = slv_addr,
			.dst_eid = tar_eid,
			.csi = 0,
			.nsid = NVME_NSID_ALL,
			.pec = pec,
			.ic = true,
			.timeout = -2,
			.thread_id = 1,
		};
		if (pthread_create(&t1, NULL, nvme_transmit_worker1, &t1_args) != 0) {
			perror("pthread_create t1");
			return 1;
		}

		// pthread_t t2;
		// struct aa_args t2_args = {
		//      .handle = handle,
		//      .verbose = verbose,
		//      .slv_addr = slv_addr,
		//      .dst_eid = tar_eid,
		//      .csi = 1,
		//      .nsid = NVME_NSID_ALL,
		//      .pec = pec,
		//      .ic = true,
		//      .timeout = -2,
		//      .thread_id = 2,
		// };
		// if (pthread_create(&t2, NULL, nvme_transmit_worker2, &t2_args) != 0) {
		//      perror("pthread_create t2");
		//      return 1;
		// }

		pthread_t t3;
		struct aa_args t3_args = {
			.handle = handle,
			.verbose = verbose,
			.slv_addr = slv_addr,
			.dst_eid = tar_eid,
			.csi = 2,
			.nsid = NVME_NSID_ALL,
			.pec = pec,
			.ic = true,
			.timeout = -2,
			.thread_id = 3,
		};
		if (pthread_create(&t3, NULL, nvme_receive_worker, &t3_args) != 0) {
			perror("pthread_create t3");
			return 1;
		}

		pthread_join(t1, NULL);
		// pthread_join(t2, NULL);
		pthread_join(t3, NULL);
#endif

		break;
	}
	case FUNC_IDX_SMB_DEVICE_POLL: {
		int ret;
		aa_i2c_slave_enable(handle, 0x3a, 0, 0);
		// while (1);
		ret = smbus_slave_poll(handle, -1, false, smbus_slave_poll_default_callback, true);
		if (ret && ret != 0xFF)
			nvme_trace(ERROR, "smbus_slave_poll (%d)\n", ret);

		break;
	}
	case FUNC_IDX_I2C_DETECT: {
		scan_i2c_bus(handle);
		break;
	}
#if 0
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
			main_trace(ERROR, "aa_i2c_file (%d)\n", ret);

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
			main_trace(ERROR, "aa_i2c_slave_poll (%d)\n", ret);

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
			main_trace(ERROR, "test_smbus_controller_target_poll (%d)\n", ret);

		break;
	}
#endif
	default:
		break;
	}

	main_exit(EXIT_SUCCESS, handle, -1, NULL);

out:
	main_exit(EXIT_FAILURE, handle, -1, NULL);
}
