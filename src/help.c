#include <stdio.h>
#include <stdlib.h>

#include "main.h"

extern const struct function_list func_list[];

void help(int func_idx)
{
	switch (func_idx) {
	case FUNC_IDX_SMB_SEND_BYTE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] %s "
		        "                [port] [target-address] [<data-byte>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -s (enable I2C slave mode)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'target-address' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (send data byte 0x5a to address 0x1d with pec):\n"
		        "  # aardvark -cu smb-send-byte 0 0x1d 0x5a\n",
		        func_list[FUNC_IDX_SMB_SEND_BYTE].name
		);
		break;
	case FUNC_IDX_SMB_BLOCK_WRITE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] %s "
		        "                [port] [target-address] [cmd-code] [<data-bytes>...]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -s (enable I2C slave mode)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'target-address' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (send data block to address 0x1a with command 0xf and pec):\n"
		        "  # aardvark -cu smb-write-file 0 0x3a 0xf test.bin\n",
		        func_list[FUNC_IDX_SMB_BLOCK_WRITE].name
		);
		break;
	case FUNC_IDX_SMB_WRITE_FILE:
		printf(
		        "Usage: aardvark [-a] [-b <bit-rate>] [-k] [-c] [-p] [-u] %s "
		        "                [port] [target-address] [cmd-code] [file-name]\n\n"
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -s (enable I2C slave mode)\n"
		        "    -u (pull-up SCL and SDA)\n\n"
		        "  'port' is an integer to indicate a valid port to use\n\n"
		        "  'target-address' is an integer (0x08 - 0x77 or 0x00 - 0x7f if '-a' is given)\n\n"
		        "Example 1 (send test.bin to address 0x3a with command 0xf and pec):\n"
		        "  # aardvark -cu smb-write-file 0 0x3a 0xf test.bin\n\n"
		        "Example 2 (send test.bin to address 0x3a with command 0xf and pec, Also, turn\n"
		        "  on the power and keep the power even if the function is complete):\n"
		        "  # aardvark -kcpu smb-write-file 0 0x3a 0xf test.bin\n",
		        func_list[FUNC_IDX_SMB_WRITE_FILE].name
		);
		break;
	default:
		printf(
		        "Usage: aardvark [<option>...] [function] [<arg>...]\n\n"
		        "  function is one of:\n"
		);
		for (int i = 0; NULL != func_list[i].name; i++) {
			printf(
			        "    %s\n",
			        func_list[i].name
			);
		}
		printf("\n");
		printf(
		        "  option is one of:\n"
		        "    -a (all range address)\n"
		        "    -b <bit-rate> (bit rate)\n"
		        "    -c (pec)\n"
		        "    -k (keep target power)\n"
		        "    -p (enable target power)\n"
		        "    -s (enable I2C slave mode)\n"
		        "    -u (pull-up SCL and SDA)\n"
		);
		break;
	}

	exit(1);
}
