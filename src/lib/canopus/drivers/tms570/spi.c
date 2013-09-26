#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/channel.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <canopus/drivers/tms570/spi.h>

#define TMS570_SPI_MAX 5 // could be reduced to 3 on Torino boards (and update idx())

typedef struct tms570_spi_isr_ctx {
	signed portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreHandle semphr;
	SpiDataStatus_t status;
	frame_t *rx, *tx;
	spiDAT1_t *dat;
	uint8_t wordsize_in_bytes;
} isr_ctx_t;
static isr_ctx_t spi_isr_ctx[TMS570_SPI_MAX] = {};

static int use_loopback = 0;

static inline uint32_t
idx(spiBASE_t *spi) {
	return spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));
}

void
spi1Init(void)
{
    /** @b initialize @b SPI1 */

    /** bring SPI out of reset */
    spiREG1->GCR0 = 1U;

    /** SPI1 master mode and clock configuration */
    spiREG1->GCR1 = (spiREG1->GCR1 & 0xFFFFFFFCU) | ((1U << 1U)  /* CLOKMOD */
                  | 1U);  /* MASTER */

    /** SPI1 enable pin configuration */
    spiREG1->INT0 = (spiREG1->INT0 & 0xFEFFFFFFU)| (0U << 24U);  /* ENABLE HIGHZ */

    /** - Delays */
    spiREG1->DELAY = (0U << 24U)  /* C2TDELAY */
                   | (1U << 16U)  /* T2CDELAY */
                   | (0U << 8U)  /* T2EDELAY */
                   | 0U;  /* C2EDELAY */

    /** - Data Format 0 */
    spiREG1->FMT0 = (60U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (1U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (62U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 1 */
    spiREG1->FMT1 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (124U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 2 */
    spiREG1->FMT2 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 3 */
    spiREG1->FMT3 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - set interrupt levels */
    spiREG1->LVL = (0U << 9U)  /* TXINT */
                 | (0U << 8U)  /* RXINT */
                 | (0U << 6U)  /* OVRNINT */
                 | (0U << 4U)  /* BITERR */
                 | (0U << 3U)  /* DESYNC */
                 | (0U << 2U)  /* PARERR */
                 | (0U << 1U) /* TIMEOUT */
                 | (0U);  /* DLENERR */

    /** - clear any pending interrupts */
    spiREG1->FLG |= 0xFFFFU;

    /** - enable interrupts */
    spiREG1->INT0 = (spiREG1->INT0 & 0xFFFF0000U)
				  | (0U << 9U)  /* TXINT */
                  | (0U << 8U)  /* RXINT */
                  | (0U << 6U)  /* OVRNINT */
                  | (0U << 4U)  /* BITERR */
                  | (0U << 3U)  /* DESYNC */
                  | (0U << 2U)  /* PARERR */
                  | (0U << 1U) /* TIMEOUT */
                  | (0U);  /* DLENERR */

    /** @b initialize @b SPI1 @b Port */

    /** - SPI1 Port output values */
    spiREG1->PCDOUT =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port direction */
    spiREG1->PCDIR  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port open drain enable */
    spiREG1->PCPDR  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port pullup / pulldown selection */
    spiREG1->PCPSL  =  1U       /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - SPI1 Port pullup / pulldown enable*/
    spiREG1->PCDIS  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /* SPI1 set all pins to functional */
    spiREG1->PCFUN  =  1U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */


    /** - Finally start SPI1 */
    spiREG1->GCR1 = (spiREG1->GCR1 & 0xFEFFFFFFU) | (1U << 24U);
}

void
spi3Init(void)
{
    /** @b initialize @b SPI3 */

    /** bring SPI out of reset */
    spiREG3->GCR0 = 1U;

    /** SPI3 master mode and clock configuration */
    spiREG3->GCR1 = (spiREG3->GCR1 & 0xFFFFFFFCU) | ((1U << 1U)  /* CLOKMOD */
                  | 1U);  /* MASTER */

    /** SPI3 enable pin configuration */
    spiREG3->INT0 = (spiREG3->INT0 & 0xFEFFFFFFU) | (0U << 24U);  /* ENABLE HIGHZ */

    /** - Delays */
    spiREG3->DELAY = (1U << 24U)  /* C2TDELAY */
                   | (2U << 16U)  /* T2CDELAY */
                   | (0U << 8U)  /* T2EDELAY */
                   | 0U;  /* C2EDELAY */

    /** - Data Format 0 */
    spiREG3->FMT0 = (8U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (1U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */


    /** - Data Format 1 */
    spiREG3->FMT1 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 2 */
    spiREG3->FMT2 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 3 */
    spiREG3->FMT3 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - set interrupt levels */
    spiREG3->LVL = (0U << 9U)  /* TXINT */
                 | (0U << 8U)  /* RXINT */
                 | (0U << 6U)  /* OVRNINT */
                 | (0U << 4U)  /* BITERR */
                 | (0U << 3U)  /* DESYNC */
                 | (0U << 2U)  /* PARERR */
                 | (0U << 1U) /* TIMEOUT */
                 | (0U);  /* DLENERR */

    /** - clear any pending interrupts */
    spiREG3->FLG |= 0xFFFFU;

    /** - enable interrupts */
    spiREG3->INT0 = (spiREG3->INT0 & 0xFFFF0000U)
				  | (0U << 9U)  /* TXINT */
                  | (0U << 8U)  /* RXINT */
                  | (0U << 6U)  /* OVRNINT */
                  | (0U << 4U)  /* BITERR */
                  | (0U << 3U)  /* DESYNC */
                  | (0U << 2U)  /* PARERR */
                  | (0U << 1U) /* TIMEOUT */
                  | (0U);  /* DLENERR */

    /** @b initialize @b SPI3 @b Port */

    /** - SPI3 Port output values */
    spiREG3->PCDOUT =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port direction */
    spiREG3->PCDIR  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port open drain enable */
    spiREG3->PCPDR  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port pullup / pulldown selection */
    spiREG3->PCPSL  =  1U       /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - SPI3 Port pullup / pulldown enable*/
    spiREG3->PCDIS  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /* SPI3 set all pins to functional */
    spiREG3->PCFUN  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 2U)  /* SCS[2] */
                    | (1U << 3U)  /* SCS[3] */
                    | (1U << 4U)  /* SCS[4] */
                    | (1U << 5U)  /* SCS[5] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - Finally start SPI3 */
    spiREG3->GCR1 = (spiREG3->GCR1 & 0xFEFFFFFFU) | (1U << 24U);

}

static retval_t
_initialize(
        const channel_driver_t * const driver)
{
	int i;
	// if initialization of spi is here, FMTx dont work
	//spi1Init();
	//spi3Init();

	for (i = 0; i < TMS570_SPI_MAX; i++) {
		spi_isr_ctx[i].xHigherPriorityTaskWoken = pdFALSE;
		vSemaphoreCreateBinary(spi_isr_ctx[i].semphr);
		xSemaphoreTake(spi_isr_ctx[i].semphr, portMAX_DELAY);
	}

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
	tms570_spi_channel_config_t *c = (tms570_spi_channel_config_t *)(channel->config);
	tms570_spi_channel_state_t *s = (tms570_spi_channel_state_t *)(channel->state);
	spiDAT1_t *dat = &s->dat;

	if (0 == c->common.transaction_timeout_ms) {
		return RV_ILLEGAL; // FIXME can it be legal? we want the caller to check the retval
	}

	s->id = idx(c->base);

	dat->CS_HOLD = c->cs_hold_enabled;
	dat->WDEL = c->transactions_wait_delay;
	dat->DFSEL = c->DFSEL;
	// chip select applies CSNR when active, so we want to put 0 on cs bit = cs pin.
	dat->CSNR = ~(1 << c->cs);

	spi_isr_ctx[s->id].wordsize_in_bytes = c->wordsize_in_bits / 8;

	if (use_loopback) {
		c->base->IOLPKTSTCR = 0x00000A00U | (Digital << 1U);
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
_transact(
        const channel_t * const channel,
        frame_t * const send_frame,
        uint32_t delay_ms,
        frame_t * const recv_frame,
        const size_t send_bytes,
        const size_t recv_bytes)
{
	tms570_spi_channel_config_t *c = (tms570_spi_channel_config_t *)(channel->config);
	tms570_spi_channel_state_t *s = (tms570_spi_channel_state_t *)(channel->state);
	uint32_t size = _frame_available_data(send_frame);//, size16;
	isr_ctx_t *ctx;
	retval_t rv = RV_ERROR;

	if (_frame_available_space(recv_frame) != size) {
		return RV_ILLEGAL;
	}

	ctx = &spi_isr_ctx[s->id];
	ctx->tx = send_frame;
	ctx->rx = recv_frame;
	ctx->status = SPI_PENDING;
	ctx->dat = &s->dat;

	/** - enable interrupts */
	c->base->INT0 |= (1U << 9U)  /* TXINT */
    			  |  (1U << 8U); /* RXINT */
	if (pdFALSE == xSemaphoreTake(ctx->semphr, c->common.transaction_timeout_ms / portTICK_RATE_MS)) {
		c->base->INT0 = (c->base->INT0 & 0x0000FFFFU) & ~(0x0300U); /* Disable Interrupt */
		ctx->tx = NULL;
		ctx->rx = NULL;
		rv = RV_TIMEOUT;
	} else if (SPI_COMPLETED == ctx->status) {
		rv = RV_SUCCESS;
	} else {
		 // FIXME rv = RV_PARTIAL?
	}

	return rv;
}

static void
spiHighInterruptLevel(spiBASE_t *base, isr_ctx_t *ctx)
{
	frame_t *f;

    if (SPI_READY == ctx->status) {
    	// spurious?
    	base->INT0 = (base->INT0 & 0x0000FFFFU) & ~(0x0300U); /* Disable Interrupt */
		return;
    }

	uint32 vec = base->INTVECT0;
	size_t n_words_to_recv = 0;
    switch(vec>>1) {
    case 0x12U: /* Receive Buffer Full Interrupt */
    	f = ctx->rx;
		if (NULL == f) {
			// FIXME disable RXINTR?
			break;
		}
		n_words_to_recv = _frame_available_space(f) / ctx->wordsize_in_bytes;
		if (n_words_to_recv){
			if (ctx->wordsize_in_bytes == 1){
				frame_put_u8_nocheck(f, base->BUF & 0x000000FFU);
			}
			else {
				frame_put_u16(f, base->BUF & 0x0000FFFFU);
			}
		}
		// just received last word
		if (n_words_to_recv <= 1){ // shouldn't be 0...
			base->INT0 = (base->INT0 & 0x0000FFFFU) & ~(0x0100U);
            ctx->rx = NULL;
		}
		break;

    case 0x14U: /* Transmit Buffer Empty Interrupt */
		{
			uint32 Chip_Select_Hold = 0;
			uint16 txdata = 0;
			f = ctx->tx;
			if (NULL == f) {
				// FIXME disable TXINTR?
				break;
			}
			size_t n_words_to_send = _frame_available_data(f) / ctx->wordsize_in_bytes;

			// Use CS_HOLD configuration (enabled or disabled) only if it's not the last word.
			// On last word, force CS_HOLD to zero.
			if (n_words_to_send == 1U) {
				Chip_Select_Hold = 0U;
			} else {
				Chip_Select_Hold = ctx->dat->CS_HOLD << 28U;
			}

			if (!n_words_to_send) {
				base->INT0 = (base->INT0 & 0x0000FFFFU) & ~(0x0200U); /* Disable Interrupt */
				ctx->tx = NULL;

			} else {
				if (1 == ctx->wordsize_in_bytes) txdata = frame_get_u8_nocheck(f);
				else txdata = frame_get_u16_nocheck(f);

				base->DAT1 = (uint32)
						((ctx->dat->DFSEL   << 24U) |
						(ctx->dat->CSNR     << 16U) |
						(ctx->dat->WDEL     << 26U) |
						(Chip_Select_Hold)          |
						(txdata));
			}
		}
    	break;

    case 0x11U: /* Error Interrupt pending */
    case 0x13U: /* Receive Buffer Overrun Interrupt */
    default: /* Clear Flags and return  */
		{
			uint32 flags = (base->FLG & 0x0000FFFFU) & (~base->LVL & 0x035FU);

			base->FLG = flags;
			//TODO error with flags
			base->INT0 = (base->INT0 & 0x0000FFFFU) & ~(0x0300U); /* Disable Interrupt */
			ctx->status = SPI_READY;
		}
    	break;
    }

    if (NULL == ctx->tx && NULL == ctx->rx) {
    	ctx->status = SPI_COMPLETED;
        xSemaphoreGiveFromISR( ctx->semphr, &ctx->xHigherPriorityTaskWoken );
    }

	portYIELD_FROM_ISR( ctx->xHigherPriorityTaskWoken );
}

#pragma CODE_STATE(mibspi1HighLevelInterrupt, 32)
#pragma INTERRUPT(mibspi1HighLevelInterrupt, IRQ)
void mibspi1HighLevelInterrupt(void)
{
	spiHighInterruptLevel(spiREG1, &spi_isr_ctx[0/*indexof(spiREG1)*/]);
}

#pragma CODE_STATE(mibspi3HighInterruptLevel, 32)
#pragma INTERRUPT(mibspi3HighInterruptLevel, IRQ)
void mibspi3HighInterruptLevel(void)
{
	spiHighInterruptLevel(spiREG3, &spi_isr_ctx[2/*indexof(spiREG3)*/]);
}

static const channel_driver_api_t tms570_spi_channel_driver_api = {
    .initialize   = &_initialize,
    .deinitialize = &_deinitialize,
    .open     = &_open,
    .close    = &_close,
    .send     = INVALID_PTRC(channel_send_t *),
    .recv     = INVALID_PTRC(channel_recv_t *),
    .transact = _transact,
};

const channel_driver_t tms570_spi_channel_driver = DECLARE_CHANNEL_DRIVER(&tms570_spi_channel_driver_api, NULL, tms570_spi_channel_driver_state_t);
