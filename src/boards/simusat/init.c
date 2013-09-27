#include <canopus/logging.h>
#include <canopus/nvram.h>

#include <canopus/board.h>

#include <canopus/drivers/i2c.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/simusat/channel_posix.h>
#include <canopus/drivers/simusat/remote.h>
#include <canopus/drivers/memory/channel_link_driver.h>
#include <canopus/drivers/flash.h>

#include <canopus/drivers/simusat/gyroscope.h>
#include <canopus/board/channels.h>

#include <canopus/subsystem/aocs/aocs.h>

#include <FreeRTOS.h>
#include <semphr.h>

#define PORT_NRW          10003
#define PORT_MTQ          10004

#include "channels.c"

const channel_t *const ch_umbilical_in  = &DECLARE_CHANNEL_FD(0);
const channel_t *const ch_umbilical_out = &DECLARE_CHANNEL_FD(1);
static const channel_t datadiscard_channel = DECLARE_CHANNEL_FD(666);

const channel_t *const ch_fpga_hub = &datadiscard_channel;
const channel_t *const ch_fpga_slave = &datadiscard_channel;

const channel_t *const ch_startracker = &datadiscard_channel;
const channel_t *const ch_nanowheel = &DECLARE_CHANNEL_TCP_CLIENT("127.0.0.1", PORT_NRW);

/* Standalone Gyroscope */
static posix_gyroscope_state_t gyroscope_0_state;
static const gyroscope_t _gyroscope_0 = {
    .base  = &posix_gyroscope_base,
    .state = (gyroscope_state_t*)&gyroscope_0_state };
const gyroscope_t * const gyroscope_0 = &_gyroscope_0;
static const gyroscope_config_t gyroscope_0_config = { /* nothing */ };

static retval_t
_init_umbilical(void)
{
    retval_t rv;

    rv = channel_open(ch_umbilical_in);
    if (rv != RV_SUCCESS) return rv;

    rv = channel_open(ch_umbilical_out);
    if (rv != RV_SUCCESS) return rv;

    log_channel_init(ch_umbilical_out, "DEBUG> ");
    log_report(LOG_GLOBAL, "CONSOLE!\n");
    (void)log_setmask(nvram.mm.logmask, true);

    return RV_SUCCESS;
}

static inline retval_t _init_network()
{
#ifdef __MINGW32__
    WSADATA wsaData;

    if (NO_ERROR == WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        return RV_ERROR;
    }
#endif

    return RV_SUCCESS;
}

static inline void
board_i2c_init()
{
    i2c_lock_init();
}

// TODO move out
retval_t
board_gyroscope_init()
{
    return gyroscope_initialize(gyroscope_0, &gyroscope_0_config);
}

#define IMU_DREADY_SIMUSAT_INTERVAL_ms ((1.0/819.2)*1000.0)

static void task_imu_dready_semaphore(void *notused) {
	while (1) {
		vTaskDelay(IMU_DREADY_SIMUSAT_INTERVAL_ms / portTICK_RATE_MS);
		imu_dready_count++;
		switch(imu_dready_count) {
		case IMU_DREADY_COUNT_SAMPLE - 1:
		case IMU_DREADY_COUNT_SAMPLE:
			xSemaphoreGive(xSemaphore_imu_dready);
			break;
		case IMU_DREADY_COUNT_SAMPLE + 1:
			imu_dready_count = 0;
			break;
		}
	}
}

static retval_t init_imu_irq_driver() {
	vSemaphoreCreateBinary(xSemaphore_imu_dready);

	imu_dready_count = 0;

	if (pdPASS != xTaskCreate(task_imu_dready_semaphore, (signed char *)"IMU DREADY simusat", 64, NULL, 4, NULL)) {
		return RV_ERROR;
	}

	return RV_SUCCESS;
}

/* scheduler enabled */
retval_t board_init_scheduler_running(void) {
    retval_t rv;
    bool success = true;

    rv = _init_network();
    success &= (rv != RV_SUCCESS);

    board_i2c_init();

    /* File Channel Link Driver */
    rv = channel_driver_initialize(&fd_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&file_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&tcp_client_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&tcp_server_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&unix_socket_client_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&unix_socket_server_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&remote_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&remote_portmapped_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_driver_initialize(&memory_channel_driver);
    success &= (rv != RV_SUCCESS);

    rv = channel_open(&memory_channel_0);
    success &= (rv != RV_SUCCESS);

    rv = channel_open(&memory_channel_1);
    success &= (rv != RV_SUCCESS);

    /* Console */
    rv = _init_umbilical();
    success &= (rv != RV_SUCCESS);

    rv = channel_open(ch_nanowheel);
    success &= (rv != RV_SUCCESS);

    rv = channel_open(ch_fpga_ctrl);
    success &= (rv != RV_SUCCESS);

    rv = init_imu_irq_driver();	/* some sort of drive? a channel? */
    success &= (rv != RV_SUCCESS);
    return RV_SUCCESS;// XXX success ? RV_SUCCESS : RV_ERROR;
}

/* no scheduler */
void board_init_scheduler_not_running() {

}
