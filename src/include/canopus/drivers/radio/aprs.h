#ifndef _APRS_H_
#define _APRS_H_

retval_t APRS_process_incomming(frame_t *iframe, frame_t *oframe);
void APRS_add_to_seen_calls_list(uint8_t *call, uint8_t ssid);
retval_t APRS_add_seen_calls_list_to_frame(frame_t *oframe);
#endif /* _APRS_H_ */
