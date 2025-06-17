#include "global.h"
#include "types.h"
#include "aardvark.h"
#include "nvme.h"
#include "nvme_cmd.h"
#include "libnvme_types.h"

#include "mctp_core.h"
#include "smbus.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#ifdef WIN32
#include <windows.h>
#endif

#if (CONFIG_AA_MULTI_THREAD)
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_tx_to_rx = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_rx_to_tx = PTHREAD_COND_INITIALIZER;
int tx_ready = 0;
int rx_ready = 0;

void *nvme_transmit_worker1(void *args)
{
	int ret;
	int count = 0;

	while (1) {
		printf("[T1] Transmit worker round #%d done\n", ++count);
		// sleep(1);
		#ifdef WIN32
		Sleep(500);
		#else
		sleep(500 * 1000);
		#endif

		ret = nvme_get_features_power_mgmt(args, NVME_GET_FEATURES_SEL_CURRENT);
		if (ret) {
			nvme_trace(ERROR, "nvme_get_features_power_mgmt (%d)\n", ret);
			return NULL;
		}

		pthread_mutex_lock(&lock);
		tx_ready = 1;
		pthread_cond_signal(&cond_tx_to_rx);
		while (rx_ready == 0) {
			pthread_cond_wait(&cond_rx_to_tx, &lock);
		}
		rx_ready = 0;
		pthread_mutex_unlock(&lock);

		ret = nvme_identify_ctrl(args);
		if (ret) {
			nvme_trace(ERROR, "nvme_identify_ctrl (%d)\n", ret);
			return NULL;
		}

		pthread_mutex_lock(&lock);
		tx_ready = 1;
		pthread_cond_signal(&cond_tx_to_rx);
		while (rx_ready == 0) {
			pthread_cond_wait(&cond_rx_to_tx, &lock);
		}
		rx_ready = 0;
		pthread_mutex_unlock(&lock);

		ret = nvme_get_log_smart(args, NVME_NSID_ALL, true);
		if (ret) {
			nvme_trace(ERROR, "nvme_get_log_smart (%d)\n", ret);
			return NULL;
		}

		pthread_mutex_lock(&lock);
		tx_ready = 1;
		pthread_cond_signal(&cond_tx_to_rx);
		while (rx_ready == 0) {
			pthread_cond_wait(&cond_rx_to_tx, &lock);
		}
		rx_ready = 0;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void *nvme_receive_worker(void *args)
{
	int ret;
	struct aa_args *_args = args;
	int count = 0;
	int timeout = _args->timeout == -2 ? 1000 : _args->timeout;

	while (1) {
		printf("[T3] Receive worker round #%d done\n", ++count);
		// Sleep(1);

		pthread_mutex_lock(&lock);
		while (tx_ready == 0) {
			pthread_cond_wait(&cond_tx_to_rx, &lock);
		}
		tx_ready = 0;
		pthread_mutex_unlock(&lock);

		while (1) {
			aa_mutex_lock(aa_mutex);
			ret = smbus_slave_poll_2(_args->handle, timeout, _args->pec, mctp_receive_packet_handle,
			                         _args->verbose);
			aa_mutex_unlock(aa_mutex);
			if (ret) {
				if (ret == 0xFF)
					break;
				nvme_trace(ERROR, "smbus_slave_poll_2 failed (%d)\n", ret);
			}
		}

		pthread_mutex_lock(&lock);
		rx_ready = 1;
		pthread_cond_signal(&cond_rx_to_tx);
		pthread_mutex_unlock(&lock);
	}

	return NULL;
}

void *nvme_transmit_worker2(void *args)
{
	int ret;
	int count = 0;

	while (1) {
		ret = nvme_get_features_power_mgmt(args, NVME_GET_FEATURES_SEL_CURRENT);
		if (ret) {
			nvme_trace(ERROR, "nvme_get_features_power_mgmt (%d)\n", ret);
			return NULL;
		}

		ret = nvme_identify_ctrl(args);
		if (ret) {
			nvme_trace(ERROR, "nvme_identify_ctrl (%d)\n", ret);
			return NULL;
		}

		ret = nvme_get_log_smart(args, NVME_NSID_ALL, true);
		if (ret) {
			nvme_trace(ERROR, "nvme_get_log_smart (%d)\n", ret);
			return NULL;
		}

		printf("[T2] Transmit round #%d done\n", ++count);
		sleep(2);
	}
	return NULL;
}
#endif
