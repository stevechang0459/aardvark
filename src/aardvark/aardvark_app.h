#ifndef AARDVARK_APP_H
#define AARDVARK_APP_H

#include "aardvark.h"

int aa_detect(void);
int aa_i2c_file(int port, u8 tar_addr, char *file_name);
void i2c_slave_poll(Aardvark handle, int timeout_ms);
int aa_i2c_slave_poll(int port, u8 dev_addr, int timeout_ms);
int aa_smbus_device_poll(int port, u8 dev_addr, int timeout_ms);
int test_smbus_controller_target_poll(int port, u8 tar_addr, u8 slv_addr, int timeout_ms);
int test_smbus_file_block_write(int port, u8 tar_addr, char *filename);

#endif // ~ AARDVARK_APP_H