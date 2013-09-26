#ifndef _CANOPUS_AX25_H
#define _CANOPUS_AX25_H

#include <canopus/types.h>
#include <canopus/frame.h>

#define AX25_HEADER_SIZE  16
#define AX25_TRAILER_SIZE 2
#define AX25_MIN_CORRECT_FRAME_LEN (AX25_HEADER_SIZE+AX25_TRAILER_SIZE)

#define AX25_CALLSIGN_LEN   6
#define AX25_MAXIMUM_ALLOWED_REPEATERS 8

#define AX25_CTRL_UI      0x03
#define AX25_PID_NOLAYER3 0xF0

/* frame sizes and constantss */

#define MAX_FRAME_BUFFER_LEN 265  /* header(8) + payload(0 to 255) + payload_chksum_a(1) + payload_chksum_b(1) */
#define CHECKSUM_SIZE          2
#define FRAME_HEADER_SIZE      8
#define MINIMUM_FRAME_SIZE     FRAME_HEADER_SIZE
#define FRAME_PAYLOAD_MAXSIZE (MAX_FRAME_BUFFER_LEN - FRAME_HEADER_SIZE - CHECKSUM_SIZE) /* total_frame_size - header_size - payload_chksum_bytes(2) */

retval_t advance_over_ax25(frame_t *frame);

#endif
