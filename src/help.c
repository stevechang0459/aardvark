#include <stdio.h>
#include <stdlib.h>

#include "main.h"

extern const struct function_list func_list[];

void help(int func_idx)
{
	const char *func_name = func_list[func_idx].name;

	switch (func_idx) {
	case FUNC_IDX_SMB_SEND_BYTE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [data]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Send data byte 0x5a to address 0x1d with pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0x5a\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_WRITE_BYTE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [<data>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Write data byte 0x5a to address 0x1d with command 0xf and pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf 0x5a\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_WRITE_WORD:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [<data>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Write data word 0x1234 to address 0x1d with command 0xf and pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf 0x1234\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_WRITE_32:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [<data>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Write double word 0x12345678 to address 0x1d with command 0xf and \n"
		        "pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf 0x12345678\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_WRITE_64:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [<data>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Write quad word 0x1234567812345678 to address 0x1d with command 0xf\n"
		        "and pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf 0x1234567812345678\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_BLOCK_WRITE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [<data>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Send data block to address 0x1d with command 0xf and pec):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf 0x12 0x34\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_PREPARE_TO_ARP:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "Example 1 (Send Prepare to ARP to inform all devices that the host is starting\n"
		        "the ARP process):\n"
		        "  # aardvark -kcpu %s 0\n\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_GET_UDID:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-d] [-k] [-p] [-u] %s [port]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -d (directed)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "Example 1 (Send Get UDID (general) to retrieve UDID from all ARP-capable and/or\n"
		        "Discoverable devices):\n"
		        "  # aardvark -kcpu %s 0\n\n"
		        "Example 2 (Send Get UDID (directed) to address 0x1d to retrieve UDID if it is\n"
		        "an ARP-capable device):\n"
		        "  # aardvark -dkcpu %s 0 0x1d\n\n"
		        , func_name, func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_RESET_DEVICE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-d] [-k] [-p] [-u] %s [port]\n"
		        "                [slv_addr]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -d (directed)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Force all non-PTA, ARP-capable devices to return to their initial \n"
		        "state):\n"
		        "  # aardvark -kcpu %s 0\n\n"
		        "Example 2 (Send a directed Reset Device ARP to address 0x1d to force a device\n"
		        "to return to its initial state if it is a non-PTA or an ARP-capable device):\n"
		        "  # aardvark -dkcpu %s 0 0x1d\n\n"
		        , func_name, func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_ASSIGN_ADDR:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [<udid bytes>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (Assign address 0x1d to a specific device):\n"
		        "  # aardvark -kcpu %s 0 0x1d <udid>...\n\n"
		        , func_name, func_name
		);
		break;
	case FUNC_IDX_SMB_WRITE_FILE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [cmd_code] [file_name]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'slv_addr' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (send test.bin to address 0x1d with command 0xf and pec):\n"
		        "  # aardvark -cu %s 0 0x1d 0xf test.bin\n\n"
		        "Example 2 (send test.bin to address 0x1d with command 0xf and pec, Also, turn\n"
		        "  on the power and keep the power even if the function is complete):\n"
		        "  # aardvark -kcpu %s 0 0x1d 0xf test.bin\n"
		        , func_name, func_name, func_name
		);
		break;
	case FUNC_IDX_TEST_MCTP:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-c] [-k] [-p] [-u] %s [port] [slv_addr]\n"
		        "                [eid]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'eid' is an integer (0x00, 0x08 - 0xfe)\n\n"
		        "Example 1 (send test.bin to address 0x1d with command 0xf and pec):\n"
		        "  # aardvark -kcpu %s 0 0x08\n\n"
		        , func_name, func_name
		);
		break;
	default:
		printf(
		        "Usage: aardvark [<option>...] [function] [<arg>...]\n\n"
		        "  function is one of:\n"
		);
		for (int i = 0; NULL != func_list[i].name; i++) {
			printf("    %s\n", func_list[i].name);
		}
		printf("\n");
		printf(
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -d (directed)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -s (enable I2C slave mode)\n"
		        "    -u (pull-up SCL and SDA)\n"
		);
		break;
	}

	exit(1);
}
