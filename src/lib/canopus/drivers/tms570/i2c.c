#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/i2c.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <canopus/drivers/tms570/i2c.h>
#include "i2c.h"


// Bibliography:
// http://www.ti.com/lit/ug/sprue11c/sprue11c.pdf

#define TMS570_I2C_TRAMSMITER_RECEIVER_BIT	9
#define TMS570_I2C_MASTER_BIT			10
#define TMS570_I2C_FREE_RUNNING_BIT		14

inline static
void setReceiverMode(i2cBASE_t *base){
	base->MDR &= ~(1 << TMS570_I2C_TRAMSMITER_RECEIVER_BIT);
}

inline static
void setTransmiterMode(i2cBASE_t *base){
	base->MDR |= (1 << TMS570_I2C_TRAMSMITER_RECEIVER_BIT);
}

inline static
void setMasterMode(i2cBASE_t *base){
	base->MDR |= (1 << TMS570_I2C_MASTER_BIT);
}

inline static
void setFreeRun(i2cBASE_t *base){
	base->MDR |= (1 << TMS570_I2C_FREE_RUNNING_BIT);
}

inline static
void resetI2C(i2cBASE_t *base){
	i2cREG1->MDR &= ~(1U << 5U);
	i2cREG1->MDR |= I2C_RESET_OUT;
}

static
void myi2cInit(void)
{
    /** @b initialize @b I2C */

    /** - i2c out of reset */
    i2cREG1->MDR = (1U << 5U);

    /** - set i2c mode */
    i2cREG1->MDR =   (0U << 15U)     /* nack mode                        */
                   | (1U << 14U)     /* free running                      */
                   |  0U     /* start condition - master mode only */
                   | (1U <<11U)     /* stop condition                     */
                   | (1U <<10U)     /* Master/Slave mode                 */
                   | (I2C_TRANSMITTER)     /* Transmitter/receiver              */
                   | (I2C_7BIT_AMODE)     /* xpanded address                   */
                   | (0 << 7U)     /* repeat mode                       */
                   | (0U << 6U)     /* digital loopback                  */
                   | (0U << 4U)     /* start byte - master only          */
                   | (0U)     /* free data format                  */
                   | (I2C_8_BIT);     /* bit count                         */


    /** - set i2c extended mode */
    i2cREG1->EMDR = (0U << 25U);

    /** - set i2c data count */
    i2cREG1->CNT = 1U;

    /** - disable all interrupts */
    i2cREG1->IMR = 0x00U;

#if 0 // (san)
// set with HCLK=100MHz, VCLK1=25MHz, so Prescale=2, ModuleClk=8.33MHz, MasterClk=25/(2+1)/(2*(412+5))=9.992KHz~=10KHz
    /** - set prescale */
    i2cREG1->PSC = 2U;

    /** - set clock rate */
    i2cREG1->CLKH = 412U; //10khz at 100MHz
    i2cREG1->CLKL = 412U;
#else // (phil)
// set with HCLK=50MHz, VCLK1=12.5MHz, so Prescale=1, ModuleClk=6.25MHz, MasterClk=12.5/(1+1)/(2*(307+6))=9.984KHz~=10KHz
// WARNING: datasheet says "The module clock frequency must be between 6.7MHz and 13.3MHz."
    /** - set prescale */
    i2cREG1->PSC = 1U;

    /** - set clock rate */
    i2cREG1->CLKH = 307U;
    i2cREG1->CLKL = 307U;
#endif

    /** - set i2c pins functional mode */
    i2cREG1->FUN = (0U);

    /** - set i2c pins default output value */
    i2cREG1->DOUT = (1U << 1U)     /* sda pin */
                  | (1U);     /* scl pin */

    /** - set i2c pins output direction */
    i2cREG1->DIR = (0U << 1U)     /* sda pin */
                 | (0U);     /* scl pin */

    /** - set i2c pins open drain enable */
    i2cREG1->ODR = (0U << 1U)     /* sda pin */
                 | (0U);     /* scl pin */

    /** - set i2c pins pullup/pulldown enable */
    i2cREG1->PD = (0U << 1U)     /* sda pin */
                | (0U);     /* scl pin */

    /** - set i2c pins pullup/pulldown select */
    i2cREG1->PSL = (1U << 1U)     /* sda pin */
                 | (1U);     /* scl pin */

    /** - set interrupt enable */
    i2cREG1->IMR    = (0U << 6U)     /* Address as slave interrupt      */
                    | (0U << 5U)     /* Stop Condition detect interrupt */
                    | (0U << 4U)     /* Transmit data ready interrupt   */
                    | (0U << 3U)     /* Receive data ready interrupt    */
                    | (0U << 2U)     /* Register Access ready interrupt */
                    | (0U << 1U)     /* No Acknowledgment interrupt    */
                    | (0U);     /* Arbitration Lost interrupt      */

    i2cREG1->MDR |= I2C_RESET_OUT; /* i2c out of reset */
}

static inline uint32 i2cIsTxReady(i2cBASE_t *base)
{
    return base->STR & I2C_TX_INT;
}

static inline uint32 i2cIsRxReady(i2cBASE_t *base)
{
    return base->STR & I2C_RX_INT;
}

static retval_t
_initialize(
        const channel_driver_t * const driver)
{
	myi2cInit();

	return RV_SUCCESS;
}

static retval_t
_deinitialize(
        const channel_driver_t * const driver)
{
	return RV_SUCCESS;
}

static retval_t
_open(
        const channel_t * const channel)
{
	tms570_i2c_channel_state_t *c_state = (tms570_i2c_channel_state_t *)(channel->state);

	// TODO: add baudrate configuration
	// Datasheet says that configuration must be done while i2c is in reset state:
	// "IRS must be 0 while the I2C module is being configured."
	//i2cSetBaudrate(c->base, c->baudrate_khz);

	c_state->debug = true; // breakpoint to change

	return RV_SUCCESS;
}

static retval_t
_close(
        const channel_t * const channel)
{
	return RV_SUCCESS;
}

static retval_t _send(
		const channel_t * const channel,
		frame_t * const send_frame,
		const size_t count)
{
	tms570_i2c_channel_config_t *c = (tms570_i2c_channel_config_t *)(channel->config);
	tms570_i2c_channel_state_t *s = (tms570_i2c_channel_state_t *)(channel->state);

	int i;
	retval_t rv;
	portTickType start, timeout_ticks;

	if (count == 0) {
		return RV_SUCCESS;
	}
	if (!frame_hasEnoughData(send_frame, count)){
		return RV_NOSPACE;
	}

    rv = i2c_lock(channel->config->lock_timeout_ms / portTICK_RATE_MS);
    if (RV_SUCCESS != rv) return rv;
    vTaskDelay(1 / portTICK_RATE_MS);
	// Reset i2c
	resetI2C(c->base);

	i2cSetSlaveAdd(c->base, c->slave_address);

	// REMEMBER! Set the i2c_master and transmitter bit before each transaction!!
	setTransmiterMode(c->base);
	setMasterMode(c->base);

	if (true == s->debug) {
		setFreeRun(c->base); // This is to enable i2c when debugging
	}

	i2cSetCount(c->base, count);
	i2cSetStop(c->base);
	i2cSetStart(c->base);

	timeout_ticks = c->common.transaction_timeout_ms / portTICK_RATE_MS;
	for (i = 0; i < count; i++) {
		start = xTaskGetTickCount();
	    while (!i2cIsTxReady(c->base))
	    {
			taskYIELD();
	        /* Wait */
			if (xTaskGetTickCount() - start > timeout_ticks) {
				i2cSetStop(c->base);
				i2c_unlock();
				return RV_TIMEOUT;
			}
		}
	    c->base->DXR = frame_get_u8_nocheck(send_frame);
	    vTaskDelay(1 / portTICK_RATE_MS);
	}
	// Wait for the transmission to complete so we can clear I2C_TX_INT from STR.
	start = xTaskGetTickCount();
    while (!i2cIsTxReady(c->base))
    {
		taskYIELD();
        /* Wait */
		if (xTaskGetTickCount() - start > timeout_ticks) {
			i2cSetStop(c->base);
			i2c_unlock();
			return RV_TIMEOUT;
		}
	}
	// Stop condition must be generated automatically here 'cause
	// the counter register falls to zero.
	c->base->STR = I2C_TX_INT;
	// TODO: add NACK check and implement zero count writes (just sending slave address)
	i2c_unlock();
	return RV_SUCCESS;
}

static retval_t _recv(
    const channel_t * const channel,
    frame_t * const recv_frame,
    const size_t count)
{
	tms570_i2c_channel_config_t *c_config = (tms570_i2c_channel_config_t *)(channel->config);

	int i;
	retval_t rv;
	portTickType start, timeout_ticks;

	if (count == 0) {
		return RV_SUCCESS;
	}
	if (_frame_available_space(recv_frame) < count) {
		return RV_NOSPACE;
	}
	rv = i2c_lock(channel->config->lock_timeout_ms / portTICK_RATE_MS);
    if (RV_SUCCESS != rv) return rv;

    vTaskDelay(1 / portTICK_RATE_MS);
	// Reset i2c
	resetI2C(c_config->base);

	i2cSetSlaveAdd(c_config->base, c_config->slave_address);

	setMasterMode(c_config->base);
	setReceiverMode(c_config->base);

	i2cSetCount(c_config->base, count);
	i2cSetStop(c_config->base);
	i2cSetStart(c_config->base);
	// Issuing a start on read mode will force the
	// generation of the clock by the TMS.
	timeout_ticks = c_config->common.transaction_timeout_ms / portTICK_RATE_MS;
	for (i = 0; i < count; i++) {
		uint8_t byte;

		start = xTaskGetTickCount();
	    while (!i2cIsRxReady(c_config->base))
	    {
			taskYIELD();
	        /* Wait */
			if (xTaskGetTickCount() - start > timeout_ticks) {
				i2cSetStop(c_config->base);
				i2c_unlock();
				return RV_TIMEOUT;
			}
		}
		byte = c_config->base->DRR;
		frame_put_u8_nocheck(recv_frame, byte);
	    vTaskDelay(1 / portTICK_RATE_MS);
	}
	// Stop condition must be generated automatically here 'cause
	// the counter register falls to zero.
	// TODO: add NACK check and implement zero count writes (just sending slave address)
	i2c_unlock();
	return RV_SUCCESS;
}

static const channel_driver_api_t tms570_i2c_channel_driver_api = {
    .initialize   = &_initialize,
    .deinitialize = &_deinitialize,
    .open     = &_open,
    .close    = &_close,
    .send     = _send,
    .recv     = _recv,
    .transact = INVALID_PTRC(channel_transact_t *),
};

const channel_driver_t tms570_i2c_channel_driver = DECLARE_CHANNEL_DRIVER(&tms570_i2c_channel_driver_api, NULL, tms570_i2c_channel_driver_state_t);
