#include <stdio.h>
#include <stdlib.h>

#include "aardvark.h"

#define BUFFER_SIZE      65535
#define SLAVE_RESP_SIZE     26
#define INTERVAL_TIMEOUT   500

static u8 data_in[BUFFER_SIZE];

static void smbus_device_poll(Aardvark handle, int timeout_ms)
{
	int trans_num = 0;
	int idle_num = 0;
	int result;
	u8 dev_addr;

	printf("Watching slave SMBus data...\n");

	// Wait for data on bus
	result = aa_async_poll(handle, timeout_ms);
	if (result == AA_ASYNC_NO_DATA) {
		printf("No data available.\n");
		return;
	}

	printf("\n");

	// Loop until aa_async_poll times out
	for (;;) {
		/**
		 * Read the I2C message.
		 *
		 * This function has an internal timeout (see datasheet), though since
		 * we have already checked for data using aa_async_poll, the timeout
		 * should never be exercised.
		 */
		if (result == AA_ASYNC_I2C_READ) {
			// Get data written by master
			int num_bytes = aa_i2c_slave_read(handle, &dev_addr,
			                                  BUFFER_SIZE, data_in);
			int i;

			if (num_bytes < 0) {
				printf("error: %s\n", aa_status_string(num_bytes));
				return;
			}

			// Dump the data to the screen
			printf("\n*** Transaction #%02d\n", trans_num);
			printf("Data read from SMbus master (%d):", num_bytes);
			for (i = 0; i < num_bytes; ++i) {
				if ((i & 0x0f) == 0) {
					printf("\n%04x:  ", i);
				}
				printf("%02x ", data_in[i] & 0xff);
				if (((i + 1) & 0x07) == 0) {
					printf(" ");
				}
			}
			printf("\n\n");
			idle_num = 0;
			++trans_num;
		} else if (result == AA_ASYNC_I2C_WRITE) {
			// Get number of bytes written to master
			int num_bytes = aa_i2c_slave_write_stats(handle);

			if (num_bytes < 0) {
				printf("error: %s\n", aa_status_string(num_bytes));
				return;
			}

			// Print status information to the screen
			printf("*** Transaction #%02d\n", trans_num);
			printf("Number of bytes written to SMbus master: %04d\n",
			       num_bytes);

			printf("\n");
		} else if (result == AA_ASYNC_SPI) {
			printf("error: non-I2C asynchronous message is pending\n");
			return;
		}

		// Use aa_async_poll to wait for the next transaction
		result = aa_async_poll(handle, INTERVAL_TIMEOUT);
		if (result == AA_ASYNC_NO_DATA) {
			// If bus idle for more than 60 seconds, just break the loop.
			if (idle_num && ((idle_num % 120) == 0)) {
				printf("Bus is idle for more than 60 seconds.\n");
				printf("No more data available from SMbus master.\n");
				// break;
			}

			++idle_num;
		}
	}
}

int aa_smbus_device_poll(int port, u8 addr, int timeout_ms)
{
	Aardvark handle;

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		printf("Unable to open Aardvark device on port %d\n", port);
		printf("Error code = %d\n", handle);
		return 1;
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_GPIO_I2C);

	// Enable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	aa_target_power(handle, AA_TARGET_POWER_BOTH);

	// Enable the slave
	aa_i2c_slave_enable(handle, addr, 0, 0);

	// Watch the I2C port
	smbus_device_poll(handle, timeout_ms);

	// Disable the Aardvark adapter's power pins.
	aa_target_power(handle, AA_TARGET_POWER_NONE);

	// Disable the slave and close the device
	aa_i2c_slave_disable(handle);
	aa_close(handle);

	return 0;
}

static void smbus_controller_target_poll(
        Aardvark handle, u8 tar_addr, int timeout_ms)
{
	int trans_num = 0;
	int idle_num = 0;
	int result;
	u8 addr;

	printf("Watching slave SMBus data...\n");

	// Wait for data on bus
	result = aa_async_poll(handle, timeout_ms);
	if (result == AA_ASYNC_NO_DATA) {
		printf("No data available.\n");
		return;
	}

	printf("\n");

	// Loop until aa_async_poll times out
	for (;;) {
		/**
		 * Read the I2C message.
		 *
		 * This function has an internal timeout (see datasheet), though since
		 * we have already checked for data using aa_async_poll, the timeout
		 * should never be exercised.
		 */
		if (result == AA_ASYNC_I2C_READ) {
			// Get data written by master
			int num_bytes = aa_i2c_slave_read(handle, &addr,
			                                  BUFFER_SIZE, data_in);
			int i;

			if (num_bytes < 0) {
				printf("error: %s\n", aa_status_string(num_bytes));
				return;
			}

			// Dump the data to the screen
			printf("\n*** Transaction #%02d\n", trans_num);
			printf("Data read from SMbus master (%d):", num_bytes);
			for (i = 0; i < num_bytes; ++i) {
				if ((i & 0x0f) == 0) {
					printf("\n%04x:  ", i);
				}
				printf("%02x ", data_in[i] & 0xff);
				if (((i + 1) & 0x07) == 0) {
					printf(" ");
				}
			}
			printf("\n\n");

			int count, num_write = 2;
			u8 data_out[2];
			data_out[0] = 0x12;
			data_out[1] = 0x34;

			// Write the data to the bus
			count = aa_i2c_write(handle, tar_addr, AA_I2C_NO_FLAGS,
			                     (u16)num_write, data_out);
			if (count < 0) {
				printf("error: %s\n", aa_status_string(count));
			} else if (count == 0) {
				printf("error: no bytes written\n");
				printf("  are you sure you have the right slave address?\n");
			} else if (count != num_write) {
				printf("error: only a partial number of bytes written\n");
				printf("  (%d) instead of full (%d)\n", count, num_write);
			}

			idle_num = 0;
			++trans_num;
		} else if (result == AA_ASYNC_I2C_WRITE) {
			// Get number of bytes written to master
			int num_bytes = aa_i2c_slave_write_stats(handle);

			if (num_bytes < 0) {
				printf("error: %s\n", aa_status_string(num_bytes));
				return;
			}

			// Print status information to the screen
			printf("*** Transaction #%02d\n", trans_num);
			printf("Number of bytes written to SMbus master: %04d\n",
			       num_bytes);

			printf("\n");
		} else if (result == AA_ASYNC_SPI) {
			printf("error: non-I2C asynchronous message is pending\n");
			return;
		}

		// Use aa_async_poll to wait for the next transaction
		result = aa_async_poll(handle, INTERVAL_TIMEOUT);
		if (result == AA_ASYNC_NO_DATA) {
			// If bus idle for more than 60 seconds, just break the loop.
			if (idle_num && ((idle_num % 120) == 0)) {
				printf("Bus is idle for more than 60 seconds.\n");
				printf("No more data available from SMbus master.\n");
				// break;
			}

			++idle_num;
		}
	}
}

int test_smbus_controller_target_poll(
        int port, u8 tar_addr, u8 dev_addr, int timeout_ms)
{
	Aardvark handle;

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		printf("Unable to open Aardvark device on port %d\n", port);
		printf("Error code = %d\n", handle);
		return 1;
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_GPIO_I2C);

	// Enable the I2C bus pullup resistors (2.2k resistors).
	// This command is only effective on v2.0 hardware or greater.
	// The pullup resistors on the v1.02 hardware are enabled by default.
	aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);

	// Enable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	aa_target_power(handle, AA_TARGET_POWER_BOTH);

	// Enable the slave
	aa_i2c_slave_enable(handle, dev_addr >> 1, 0, 0);

	// Watch the I2C port
	smbus_controller_target_poll(handle, tar_addr >> 1, timeout_ms);

	// Disable the Aardvark adapter's power pins.
	aa_target_power(handle, AA_TARGET_POWER_NONE);

	// Disable the slave and close the device
	aa_i2c_slave_disable(handle);
	aa_close(handle);

	return 0;
}
