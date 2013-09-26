#ifndef _CANOPUS_SUBSYSTEM_PAYLOAD_H_
#define _CANOPUS_SUBSYSTEM_PAYLOAD_H_

#include <canopus/frame.h>

extern subsystem_t SUBSYSTEM_PAYLOAD;

typedef struct nvram_payload_t {
    bool experiments_enabled;
    uint16_t total_run;
    uint16_t total_failed_count;
    int16_t last_run_test;
} nvram_payload_t;

retval_t PAYLOAD_transact_uart(uint8_t uart, frame_t *iframe, frame_t *oframe, bool lock, bool unlock);
retval_t PAYLOAD_svip_transact(frame_t *iframe, frame_t *oframe, bool lock, bool unlock);
retval_t PAYLOAD_start_experiment(uint16_t experiment_number, frame_t *oframe);
retval_t PAYLOAD_end_experiment(uint16_t experiment_number, frame_t *oframe);
retval_t PAYLOAD_get_latest_experiment_results(frame_t *oframe);
bool PAYLOAD_is_there_any_experiment_result();
bool PAYLOAD_is_there_any_image_fragment();
retval_t PAYLOAD_get_latest_image_fragment(frame_t *oframe);

#endif
