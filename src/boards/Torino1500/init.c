#include <canopus/types.h>
#include <canopus/board.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/tms570/uart.h>
#include <canopus/drivers/tms570/spi.h>
#include <canopus/drivers/tms570/i2c.h>
#include <canopus/drivers/tms570/spi.h>
#include <canopus/drivers/magnetorquer.h>
#include <canopus/drivers/i2c.h>
#include <canopus/drivers/leds.h>
#include <canopus/board/channels.h>
#include <canopus/board/adc.h>
#include <canopus/logging.h>
#include <canopus/nvram.h>

#include "gio.h"
#include "het.h"
#include "sci.h"
#include "i2c.h"
#include "adc.h"


static void
send_identification_string_on_all_uarts_at_default_baudrate()
{
	int i;

	/* disable INTR */
	sciDisableNotification(scilinREG, SCI_TX_INT|SCI_RX_INT);
	sciDisableNotification(sciREG, SCI_TX_INT|SCI_RX_INT);

	/* send polling */
	for (i = 0; i < 4; i++) {
		sciSend(scilinREG, 5, "lin\r\n");
		sciSend(sciREG, 5, "sci\r\n");
	}

	/* re-enable INTR */
	sciEnableNotification(scilinREG, SCI_TX_INT|SCI_RX_INT);
	sciEnableNotification(sciREG, SCI_TX_INT|SCI_RX_INT);
}

void
board_init_scheduler_not_running(void)
{
	gioInit();
	hetInit();
	pwm_init();
	sciInit();
	i2cInit();
	adcInit();

	(void)leds_init();
	spi1Init();
	spi3Init();

	// san: cosas bajo nivel necesarias antes del scheduler

	//send_identification_string_on_all_uarts_at_default_baudrate();
}

static inline bool
board_uart_init()
{
    retval_t rv;
    bool success = 1;

    rv = channel_driver_initialize(&tms570_sci_channel_driver);
    success &= RV_SUCCESS == rv;

    /* radio */
    rv = channel_open(ch_lithium);
    success &= RV_SUCCESS == rv;

    /* umbilical */
    rv = channel_open(ch_umbilical_in);
    success &= RV_SUCCESS == rv;

    rv = channel_open(ch_umbilical_out);
    //success &= RV_SUCCESS == rv;

    /* console */
    log_channel_init(ch_umbilical_out, "DEBUG> ");
    log_report(LOG_GLOBAL, "CONSOLE!\r\n");
    (void)log_setmask(nvram.mm.logmask, true);

    return success;
}

static inline bool board_spi_init() {
    retval_t rv;
    bool success = 1;

    rv = channel_driver_initialize(&tms570_spi_channel_driver);
    success &= RV_SUCCESS == rv;

    return success;
}

static inline bool
board_i2c_init()
{
    retval_t rv;
    bool success = 1;

    i2c_lock_init();
    rv = channel_driver_initialize(&tms570_i2c_channel_driver);
    success &= RV_SUCCESS == rv;

    return success;
}


static inline bool
board_gpio_init()
{
	vSemaphoreCreateBinary(xSemaphore_imu_dready);
	xSemaphoreTake(xSemaphore_imu_dready, portMAX_DELAY);
	gioEnableNotification(GPIO_DREADY_PORT, GPIO_DREADY_BIT);
	return true;
}
static inline bool
board_adc_init()
{
	adc_init();
	return true;
}

retval_t
board_init_scheduler_running(void)
{
	// cosas q necesitan scheduler: mutex, semphr, delay...

	board_uart_init();
	board_spi_init();
	board_i2c_init();
	board_gpio_init();
	board_adc_init();

	return RV_SUCCESS;
}
