/*=========================================================================
| (c) 2004-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aai2c_slave.c
|--------------------------------------------------------------------------
| Configure the device as an I2C slave and watch incoming data.
|--------------------------------------------------------------------------
| Redistribution and use of this file in source and binary forms, with
| or without modification, are permitted.
|
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
| FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
| COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
| CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
| ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.
 ========================================================================*/

#include <stdio.h>
#include <stdlib.h>

#include "aardvark.h"

#define BUFFER_SIZE      65535
#define SLAVE_RESP_SIZE     26
#define INTERVAL_TIMEOUT   500

static u8 data_in[BUFFER_SIZE];

static void dump(Aardvark handle, int timeout_ms)
{
	int trans_num = 0;
	int idle_num = 0;
	int result;
	u8 addr;

	printf("Watching slave I2C data...\n");

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
			printf("Data read from master:");
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
			++trans_num;
			idle_num = 0;
		} else if (result == AA_ASYNC_I2C_WRITE) {
			// Get number of bytes written to master
			int num_bytes = aa_i2c_slave_write_stats(handle);

			if (num_bytes < 0) {
				printf("error: %s\n", aa_status_string(num_bytes));
				return;
			}

			// Print status information to the screen
			printf("*** Transaction #%02d\n", trans_num);
			printf("Number of bytes written to master: %04d\n", num_bytes);

			printf("\n");
		} else {
			++idle_num;
			// printf("error: non-I2C asynchronous message is pending\n");
			// return;
		}


		// Use aa_async_poll to wait for the next transaction
		result = aa_async_poll(handle, INTERVAL_TIMEOUT);
		if (result == AA_ASYNC_NO_DATA) {
			if (idle_num && ((idle_num % 10) == 0))
				printf("No more data available from I2C master.\n");

			// break;
		}
	}
}

int aa_i2c_slave(int port, u8 addr, int timeout_ms)
{
	Aardvark handle;
	u8 slave_resp[SLAVE_RESP_SIZE];
	int i;

	// Open the device
	handle = aa_open(port);
	if (handle <= 0) {
		printf("Unable to open Aardvark device on port %d\n", port);
		printf("Error code = %d\n", handle);
		return 1;
	}

	// Ensure that the I2C subsystem is enabled
	aa_configure(handle, AA_CONFIG_SPI_I2C);

	// Disable the Aardvark adapter's power pins.
	// This command is only effective on v2.0 hardware or greater.
	// The power pins on the v1.02 hardware are not enabled by default.
	aa_target_power(handle, AA_TARGET_POWER_BOTH);

	// Set the slave response; this won't be used unless the master
	// reads bytes from the slave.
	for (i = 0; i < SLAVE_RESP_SIZE; ++i) {
		slave_resp[i] = 'A' + i;
	}

	aa_i2c_slave_set_response(handle, SLAVE_RESP_SIZE, slave_resp);

	// Enable the slave
	aa_i2c_slave_enable(handle, addr, 0, 0);

	// Watch the I2C port
	dump(handle, timeout_ms);

	// Disable the slave and close the device
	aa_i2c_slave_disable(handle);
	aa_close(handle);

	return 0;
}
