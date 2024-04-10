#ifndef AARDVARK_APP_H
#define AARDVARK_APP_H

#include "aardvark.h"

extern int aa_detect(void);
extern int aa_i2c_file(int port, u8 tar_addr, char *file_name);
extern int aa_i2c_slave_poll(int port, u8 dev_addr, int timeout_ms);
extern int aa_smbus_device_poll(int port, u8 dev_addr, int timeout_ms);
extern int test_smbus_controller_target_poll(int port, u8 tar_addr, u8 slv_addr, int timeout_ms);
extern int test_smbus_file_block_write(int port, u8 tar_addr, char *filename);

#endif // ~ AARDVARK_APP_H