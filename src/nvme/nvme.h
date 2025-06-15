#ifndef NVME_H
#define NVME_H

#include "global.h"
#include "types.h"
#include <stdint.h>
#include <stdbool.h>

#define NVME_DEFAULT_IOCTL_TIMEOUT 0

#define NVME_TRACE_FILTER ( \
        BITLSHIFT(1, ERROR) | \
        BITLSHIFT(1, WARN)  | \
        BITLSHIFT(1, DEBUG) | \
        BITLSHIFT(1, INFO)  | \
        BITLSHIFT(1, INIT))

extern const char *nvme_trace_header[];

#define nvme_trace(type, ...) \
do { \
        if (BITLSHIFT(1, type) & NVME_TRACE_FILTER) { \
        fprintf(stderr, "%s", nvme_trace_header[type]); \
        fprintf(stderr, __VA_ARGS__);} \
} while (0)


#if (CONFIG_AA_MULTI_THREAD)
void *nvme_transmit_worker1(void *args);
void *nvme_transmit_worker2(void *args);
void *nvme_receive_worker(void *args);
#endif


#endif // NVME_H