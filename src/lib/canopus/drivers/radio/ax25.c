#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/frame.h>
//#include <canopus/logging.h>
#include <canopus/drivers/radio/ax25.h>
#include <canopus/drivers/radio/aprs.h>

/**
 * advance frame's position to the start of the payload field
 * skiping the ax25 header
 */
retval_t
advance_over_ax25(frame_t *frame) {

    bool more_addresses;
    uint8_t u8;
    uint8_t call[AX25_CALLSIGN_LEN];

    if (!frame_hasEnoughData(frame, AX25_MIN_CORRECT_FRAME_LEN)) {
        return RV_ILLEGAL;
    }

    /* skip over callsign fields */
    frame_advance_nocheck(frame, (AX25_CALLSIGN_LEN + 1));
    frame_get_data_nocheck(frame, &call, AX25_CALLSIGN_LEN);
    u8 = frame_get_u8_nocheck(frame);

    APRS_add_to_seen_calls_list(call, u8);

    more_addresses = !(u8 & 0x01);

    while (more_addresses) {
        if (!frame_hasEnoughData(frame, AX25_CALLSIGN_LEN + 1)) {
            return RV_ILLEGAL;
        }
        frame_get_data_nocheck(frame, &call, AX25_CALLSIGN_LEN);
        more_addresses = !(frame_get_u8_nocheck(frame) & 0x01);

        /* log_report_fmt(LOG_RADIO, "RPT: %s-%s", call, u8); */
    }

    u8 = frame_get_u8_nocheck(frame);
    if (AX25_CTRL_UI != u8) {
        return RV_ILLEGAL;
    }

    u8 = frame_get_u8_nocheck(frame);
    if (AX25_PID_NOLAYER3 != u8) {
        return RV_ILLEGAL;
    }

    /* here would come CRC bytes, we skip the check */
    frame->size -= sizeof(uint16_t) + sizeof(uint16_t);

    return RV_SUCCESS;
}
