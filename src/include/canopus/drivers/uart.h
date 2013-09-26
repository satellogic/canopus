#ifndef _CANOPUS_DRIVERS_UART_H_
#define _CANOPUS_DRIVERS_UART_H_

/*
 * Universal Asynchronous Receiver/Transmitter
 *
 * If we plan to use the Synchronous mode (provided by USARTs) we might rename this file.
 * Some manufacturers (Motorola, Texas Instruments) identify these devices as SCI
 * (Serial Communications Interface) which might be a better standard.
 *
 * Note: In our boards we only use the Rx/Tx lines (no control lines).
 *
 */

#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/board/uart.h>

typedef enum uart_mode {
	/* blocking */
	UART_MODE_POLL,
	/* non-blocking */
	UART_MODE_INTERRUPT,
	UART_MODE_DMA
} uart_mode_t;

typedef enum uart_parity_mode {
	UART_PARITY_NONE,	/* no parity (disabled) */
	UART_PARITY_EVEN,
	UART_PARITY_ODD
} uart_parity_mode_t;

typedef enum uart_stopbit_count {
	UART_STOPBIT_1,		/* 1   */
	UART_STOPBIT_1_5,	/* 1.5 */
	UART_STOPBIT_2		/* 2   */
} uart_stopbit_count_t;

typedef struct uart_config {
	uart_mode_t hw_mode;
	uint32_t baudrate_hz;		/* the speed or bits per second of the line */
	endian_t endianness;		/* the order in which the bits are sent, standard is little endian */
	uint8_t databits_size;		/* the number of bits per character */
	uart_parity_mode_t parity_mode;
	uint8_t stopbit_count;		/* the number of stop bits sent */
	/* FIXME: full/half duplex? */
} uart_config_t;

#endif

