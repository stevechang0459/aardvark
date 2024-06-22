#ifndef MAIN_H
#define MAIN_H

#define MAIN_TRACE_FILTER (0 \
        | LSHIFT(ERROR) \
        | LSHIFT(WARN) \
        | LSHIFT(DEBUG) \
        | LSHIFT(INFO) \
        | LSHIFT(INIT) \
        )

#if 1
extern const char *main_trace_header[];

#define main_trace(type, ...) \
do { \
	if (LSHIFT(type) & MAIN_TRACE_FILTER) \
        fprintf(stderr, "%s", main_trace_header[type]); \
        fprintf(stderr, __VA_ARGS__); \
} while (0)
#else
#define main_trace(type, ...) { \
if (LSHIFT(type) & MAIN_TRACE_FILTER) \
        fprintf(stderr, "[%s]: ", #type); \
        fprintf(stderr, __VA_ARGS__);}
#endif

enum function_index {
	FUNC_IDX_NULL = -1,
	FUNC_IDX_DETECT,
	// Bus Protocols
	FUNC_IDX_SMB_SEND_BYTE,
	FUNC_IDX_SMB_WRITE_BYTE,
	FUNC_IDX_SMB_WRITE_WORD,
	FUNC_IDX_SMB_WRITE_32,
	FUNC_IDX_SMB_WRITE_64,
	FUNC_IDX_SMB_BLOCK_WRITE,
	// SMBus Address Resolution Protocol
	FUNC_IDX_SMB_PREPARE_TO_ARP,
	FUNC_IDX_SMB_GET_UDID,
	FUNC_IDX_SMB_RESET_DEVICE,
	FUNC_IDX_SMB_ASSIGN_ADDR,
	// Application
	FUNC_IDX_SMB_WRITE_FILE,
	FUNC_IDX_TEST_MCTP,

	// FUNC_IDX_I2C_MASTER_WRITE,
	// FUNC_IDX_I2C_MASTER_READ,
	// FUNC_IDX_I2C_MASTER_WRITE_FILE,
	// FUNC_IDX_I2C_SLAVE_POLL,
	// FUNC_IDX_SMB_MASTER_BLOCK_READ,
	// FUNC_IDX_SMB_DEVICE_POLL,
	// FUNC_IDX_TEST,
	// FUNC_IDX_TEST2,
	// FUNC_IDX_SMB_ARP_EXEC,
	FUNC_IDX_MAX
};

struct function_list {
	const char *name;
	enum function_index func_idx;
};

#endif // ~ MAIN_H
