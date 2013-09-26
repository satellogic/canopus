#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/channel.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <canopus/drivers/tms570/uart.h>
#include "sci.h"

#define RX_QUEUE_SIZE 256
/* #define */ static int TX_BLOCKTIME_MIN = 10;

/* #define */ static int SOME_MULT = 2;
#define FAST_DIVISOR 1024
#define MS_PER_SEC 1000
#define BITS_PER_BYTE_USING_8N1 (1 + 8 + 0 + 1)

typedef struct tms570_sci_isr_ctx {
	signed portBASE_TYPE xHigherPriorityTaskWoken;
	struct {
		xSemaphoreHandle semphr;
		frame_t *frame;
	} tx;
	struct {
		xQueueHandle queue;
	} rx;
} isr_ctx_t;
static isr_ctx_t sci_isr_ctx[TMS570_SCI_MAX] = {};

static inline uint32_t
idx(sciBASE_t *base) {
	return base == sciREG ? 0U : 1U;
}

static retval_t
_initialize(
        const channel_driver_t * const driver)
{
	int i;
	signed portBASE_TYPE rv;

	for (i = 0; i < TMS570_SCI_MAX; i++) {
		sci_isr_ctx[i].xHigherPriorityTaskWoken = pdFALSE;
		vSemaphoreCreateBinary(sci_isr_ctx[i].tx.semphr);
		assert(NULL != sci_isr_ctx[i].tx.semphr);
		rv = xSemaphoreTake(sci_isr_ctx[i].tx.semphr, portMAX_DELAY);
		assert(pdTRUE == rv);

		sci_isr_ctx[i].rx.queue = xQueueCreate( RX_QUEUE_SIZE, sizeof(uint8_t) );
		assert(NULL != sci_isr_ctx[i].rx.queue);
	}

	sciInit();

	// FIXME IRQ should be disabled until we open the channel, fix sciInit()?
	sciREG->CLRINT = SCI_TX_INT|SCI_RX_INT;
	scilinREG->CLRINT = SCI_TX_INT|SCI_RX_INT;

	return RV_SUCCESS;
}

static retval_t
_deinitialize(
        const channel_driver_t * const driver)
{
	// TODO vSemaphoreDelete (MPU calls not working)

	return RV_SUCCESS;
}

static retval_t
_open(
        const channel_t * const channel)
{
	tms570_sci_channel_config_t *c = (tms570_sci_channel_config_t *)(channel->config);
	tms570_sci_channel_state_t *s = (tms570_sci_channel_state_t *)(channel->state);
	float64 v;

	if (0 != c->interrupt_bits && 0 == c->common.transaction_timeout_ms) {
		return RV_ILLEGAL;
	}
	if (0 == c->baudrate) { // FIXME check legal values?
		return RV_ILLEGAL;
	}

	s->id = idx(c->base);

	sciSetBaudrate(c->base, c->baudrate);

	v = FAST_DIVISOR * MS_PER_SEC * BITS_PER_BYTE_USING_8N1;
	s->txBlockTimePerByte = (uint32_t)((v / c->baudrate)) / portTICK_RATE_MS;

	if (c->interrupt_bits & SCI_RX_INT) {
		c->base->SETINT = SCI_RX_INT;
	}

	return RV_SUCCESS;
}

static retval_t
_close(
        const channel_t * const channel)
{
	return RV_SUCCESS;
}

static retval_t
_send(
        const channel_t * const channel,
        frame_t * const send_frame,
        const size_t count)
{
	tms570_sci_channel_config_t *c = (tms570_sci_channel_config_t *)(channel->config);
	tms570_sci_channel_state_t *s = (tms570_sci_channel_state_t *)(channel->state);
	isr_ctx_t *ctx = &sci_isr_ctx[s->id];

	if (c->interrupt_bits & SCI_TX_INT) {
		if (0 == c->common.transaction_timeout_ms) {
			return RV_ILLEGAL; // XXX
		}
		portTickType xBlockTime = TX_BLOCKTIME_MIN + (_frame_available_data(send_frame) * s->txBlockTimePerByte * SOME_MULT / FAST_DIVISOR);

		ctx->tx.frame = send_frame;
		/* start transmit by sending first byte */
		c->base->TD     = frame_get_u8_nocheck(send_frame);
		c->base->SETINT = SCI_TX_INT;

		// TODO if (send_frame->timeout == 0) c->common.transaction_timeout_ms
		if (pdFALSE == xSemaphoreTake(ctx->tx.semphr, xBlockTime)) {
			c->base->CLRINT = SCI_TX_INT; // disable interrupt
			ctx->tx.frame = NULL;
			return RV_TIMEOUT;
		}
		// TODO partial?
	} else {
		do {
			while ((c->base->FLR & SCI_TX_INT) == 0U) {
				taskYIELD();
				// TODO timeout
			}
			c->base->TD = frame_get_u8_nocheck(send_frame);
		} while (_frame_available_data(send_frame));
	}

	return RV_SUCCESS;
}

static retval_t
_recv(
        const channel_t * const channel,
        frame_t * const recv_frame,
        const size_t count)
{
	tms570_sci_channel_config_t *c = (tms570_sci_channel_config_t *)(channel->config);
	tms570_sci_channel_state_t *s = (tms570_sci_channel_state_t *)(channel->state);
	isr_ctx_t *ctx = &sci_isr_ctx[s->id];

	if (c->interrupt_bits & SCI_RX_INT) {
		portTickType xBlockTimeout_ms = recv_frame->timeout;
		portBASE_TYPE os_rv;
		uint8_t next_byte;
		boolean got_one;

		if (!frame_hasEnoughSpace(recv_frame, 1)) return RV_NOSPACE;

		if (0 == xBlockTimeout_ms) {
			xBlockTimeout_ms = c->common.transaction_timeout_ms;
		}

		got_one = false;
		while (frame_hasEnoughSpace(recv_frame, 1)) {
			os_rv = xQueueReceive(ctx->rx.queue, &next_byte, xBlockTimeout_ms / portTICK_RATE_MS);
			if (pdTRUE != os_rv) break;

			got_one = true;
			(void /* frame_hasEnoughSpace(..., 1) */)frame_put_u8(recv_frame, next_byte);
		}

		if (!got_one) {
			return RV_TIMEOUT;
		}

		if (frame_hasEnoughSpace(recv_frame, 1)) {
			return RV_PARTIAL;
		}
	} else {
		while (frame_hasEnoughSpace(recv_frame, 1)) {
			while ((c->base->FLR & SCI_RX_INT) == 0U) {
				taskYIELD();
				// TODO timeout
			}
			frame_put_u8_nocheck(recv_frame, c->base->RD & 0x000000FFU);
		};
	}

	return RV_SUCCESS;
}

enum sci_intvec_e {
	SCI_RX_INTVEC = 11U,
	SCI_TX_INTVEC = 12U,
};

static void
sciHighLevelISR(sciBASE_t *base, isr_ctx_t *ctx)
{
	uint32 flags = base->INTVECT0;
	frame_t *f;

	switch (flags) {
	case SCI_TX_INTVEC:
		f = ctx->tx.frame;
		if (NULL == f) {
			/* we got here with xHigherPriorityTaskWoken=1 while debugging */
			base->CLRINT = SCI_TX_INT; // disable interrupt
			break;
		}
		if (_frame_available_data(f)) {
			// send next byte, stay in interrupt mode
			base->TD = frame_get_u8_nocheck(f);
		} else {
			// disable interrupt
			base->CLRINT = SCI_TX_INT;
			ctx->tx.frame = NULL;
		    xSemaphoreGiveFromISR( ctx->tx.semphr, &ctx->xHigherPriorityTaskWoken );
		}
	    break;
	default:
        /* phantom interrupt, clear flags and return */
		base->FLR = ~base->SETINTLVL & 0x07000303U;
	}

	portYIELD_FROM_ISR( ctx->xHigherPriorityTaskWoken );
}

static void
sciLowLevelISR(sciBASE_t *base, isr_ctx_t *ctx)
{
	uint32 flags = base->INTVECT1;
	uint8_t c;
	portBASE_TYPE rv;

	switch (flags) {
	case SCI_RX_INTVEC:
		if (NULL == ctx->rx.queue) {
			// should not happen... except if debugging and double sciInit() call maybe?
			base->CLRINT = SCI_RX_INT;
			break;
		}
		c = base->RD & 0x000000FFU; // empty shiftreg
		// check Rx error flags
		flags = base->FLR;
		if (flags & (SCI_FE_INT | SCI_OE_INT |SCI_PE_INT | SCI_BREAK_INT)) {
			// clear error flags
			base->FLR = SCI_FE_INT | SCI_OE_INT | SCI_PE_INT | SCI_BREAK_INT;
			// TODO ? ctx->rx.error++;
			break;
		}

		rv = xQueueSendFromISR( ctx->rx.queue, &c, &ctx->xHigherPriorityTaskWoken );
		if (errQUEUE_FULL == rv) {
			// FIXME disable RX?
		}
	    break;
	default:
        /* phantom interrupt, clear flags and return */
		base->FLR = ~base->SETINTLVL & 0x07000303U;
	}

	portYIELD_FROM_ISR( ctx->xHigherPriorityTaskWoken );
}


#pragma CODE_STATE(sciHighLevelInterrupt, 32)
#pragma INTERRUPT(sciHighLevelInterrupt, IRQ)
void sciHighLevelInterrupt(void)
{
	sciHighLevelISR(sciREG, &sci_isr_ctx[0/*indexof(sciREG)*/]);
}

#pragma CODE_STATE(sciLowLevelInterrupt, 32)
#pragma INTERRUPT(sciLowLevelInterrupt, IRQ)
void sciLowLevelInterrupt(void)
{
	sciLowLevelISR(sciREG, &sci_isr_ctx[0/*indexof(sciREG)*/]);
}

#pragma CODE_STATE(linHighLevelInterrupt, 32)
#pragma INTERRUPT(linHighLevelInterrupt, IRQ)
void linHighLevelInterrupt(void)
{
	sciHighLevelISR(scilinREG, &sci_isr_ctx[1/*indexof(scilinREG)*/]);
}

#pragma CODE_STATE(linLowLevelInterrupt, 32)
#pragma INTERRUPT(linLowLevelInterrupt, IRQ)
void linLowLevelInterrupt(void)
{
	sciLowLevelISR(scilinREG, &sci_isr_ctx[1/*indexof(scilinREG)*/]);
}

static const channel_driver_api_t tms570_sci_channel_driver_api = {
    .initialize   = &_initialize,
    .deinitialize = &_deinitialize,
    .open     = &_open,
    .close    = &_close,
    .send     = &_send,
    .recv     = &_recv,
    .transact = INVALID_PTRC(channel_transact_t *),
};

const channel_driver_t tms570_sci_channel_driver = DECLARE_CHANNEL_DRIVER(&tms570_sci_channel_driver_api, NULL, tms570_sci_channel_driver_state_t);
