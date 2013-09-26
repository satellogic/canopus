#include <canopus/frame.h>
#include <canopus/types.h>
#include <string.h>
#include <canopus/drivers/radio/ax25.h>
#include <canopus/drivers/radio/aprs.h>

#define SSID_LEN 2
#define SPACE_LEN 1
#define MAX_SEEN_CALLS	10

#define ENTRY_LEN (AX25_CALLSIGN_LEN+SSID_LEN+SPACE_LEN)
static char list_of_seen_calls[ENTRY_LEN * MAX_SEEN_CALLS + 1] = {0};

static
retval_t APRS_cmd_ping(frame_t *oframe) {
	retval_t rv = RV_ILLEGAL;
	FUTURE_HOOK_2(APRS_cmd_ping, oframe, &rv);
	return rv;
}

static
retval_t APRS_cmd_dx(frame_t *oframe) {
	return APRS_add_seen_calls_list_to_frame(oframe);
}

static
retval_t APRS_cmd_aprss(frame_t *oframe) {
	char msg[] = "Manolito ad astra! 73 to LU1VZ, LU7AA, and all my HAM friends!";
	return frame_put_data(oframe, msg, sizeof(msg)-1);
}

retval_t APRS_process_incomming(frame_t *iframe, frame_t *oframe) {
	void *data;
	retval_t rv;

	rv = frame_get_data_pointer(iframe, &data, 1);
	SUCCESS_OR_RETURN(rv);

	if (0 == strncmp(data, "?PING?", 6)) return APRS_cmd_ping(oframe);
	if (0 == strncmp(data, "?DX?", 4)) return APRS_cmd_dx(oframe);
	if (0 == strncmp(data, "?APRSS?", 7)) return APRS_cmd_aprss(oframe);
	rv = RV_ILLEGAL;
	FUTURE_HOOK_3(APRS_commands, data, oframe, &rv);
	return rv;
}

void APRS_add_to_seen_calls_list(uint8_t *call, uint8_t ssid) {
	static uint32_t list_write_head = 0;
	uint8_t callsig[ENTRY_LEN+1];
	int i;

	for (i=0;i<AX25_CALLSIGN_LEN;i++) {
		callsig[i] = call[i] >> 1;
	}
	ssid = (ssid & 0x0f) >> 1;
	if (ssid != 0) {
		callsig[i++] = '-';
		callsig[i++] = '0'+ssid;
	} else {
		callsig[i++] = ' ';
		callsig[i++] = ' ';
	}
	callsig[i++] = ' ';
	callsig[i++] = 0;

	if (NULL != strstr(list_of_seen_calls, (char*)callsig)) {
		return;
	}

	if (list_write_head > sizeof(list_of_seen_calls)-sizeof(callsig)) {
		list_write_head = 0;
	}

	memcpy(&list_of_seen_calls[list_write_head], callsig, ENTRY_LEN);

	list_write_head += ENTRY_LEN;
}

retval_t APRS_add_seen_calls_list_to_frame(frame_t *oframe) {
	return frame_put_data(oframe, &list_of_seen_calls, sizeof(list_of_seen_calls));
}

#undef SSID_LEN
#undef SPACE_LEN
#undef MAX_SEEN_CALLS
