#ifndef _CANOPUS_SUBSYSTEM_CDH_H_
#define _CANOPUS_SUBSYSTEM_CDH_H_

#include <canopus/subsystem/subsystem.h>
#include <canopus/drivers/radio/lithium.h>

extern subsystem_t SUBSYSTEM_CDH;

#define CDH_ANS_TESTING_NO_SPACE     0xFF
#define CDH_ANS_TESTING_BAD_SEQUENCE 0xFE
#define CDH_ANS_TESTING_BAD_MAC	     0xFD
#define CDH_ANS_TESTING_BAD_SS	     0xFC

#define ANTENNA_DEPLOY_INITIAL_DELAY_s	(18*60)

#define CDH_PACKET_HEADER_SIZE      (3  /* MAC */   \
                                    +3  /* SEQNUM */\
                                    +1  /* SSID */  \
                                    +1) /* CMD */

#define CDH_SEQUENCE_NUMBER_BEACON			0xFFFFF0
#define CDH_SEQUENCE_NUMBER_BEACON_ASCII	0xFFFFF2
#define CDH_SEQUENCE_NUMBER_BEACON_SHORT	0xFFFFF3
#define CDH_SEQUENCE_NUMBER_TEST_RESULTS	0xFFFFF4
#define CDH_SEQUENCE_NUMBER_IMAGE_FRAGMENT	0xFFFFF5
#define CDH_MAX_SEQUENCE_NUMBER				(CDH_SEQUENCE_NUMBER_BEACON-1)

#define CDH_ANTENNA_DEPLOY_DELAY_INTERVAL_s			10
#define CDH_ANTENNA_ARMS_COUNT						4
#define CDH_ANTENNA_DEPLOY_MAX_TRIES				16

#define CDH_TELECOMMAND_KEY_SIZE    32

#define CDH_BEACON_MAX_BATTERY_VOLTAGE	8.4f
#define CDH_BEACON_MIN_BATTERY_VOLTAGE	6.4f
#define CDH_BEACON_MS_PER_VOLT			10000
#define CDH_BEACON_MIN_INTERVAL_MS		13000
#define CDH_BEACON_MAX_DELAY_S			(20*60)
#define CDH_BEACON_ASCII_EVERY			11
#define CDH_BEACON_IS_ASCII(_n_)				(0 == _n_)
#define CDH_BEACON_IS_RESULTS(_n_)				(3 == _n_)
#define CDH_BEACON_IS_IMAGE(_n_)				(7 == _n_)
#define MAX_BEACON_SIZE		250

enum {
    FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_IGNORE       = 0,
    FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_RESET_RADIO  = 2,
    FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_RESET_ALL    = 7,
};

typedef struct cdh_subsystem_state_t {
    subsystem_state_t subsystem_state;
    xTaskHandle rx_task_handle;
    xTaskHandle beacon_update_task_handle;
    xTaskHandle antenna_deploy_task_handle;
    frame_pool_t *cdh_frame_pool;
} cdh_subsystem_state_t;

typedef struct nvram_cdh_t {
    uint32_t last_seen_sequence_number;
    bool persist_sequence_number;
    uint16_t command_response_delay_ms;
    uint8_t telecommand_key[CDH_TELECOMMAND_KEY_SIZE];
    uint32_t FDIR_CDH_LAST_COMMAND_TIMEOUTs;
    uint8_t FDIR_CDH_last_command_timeout_mask;
    uint32_t one_time_radio_silence_time_s;

    /* Radio Configuration */
    lithium_configuration_t default_lithium_configuration;
    bool change_bps_on_every_boot;

    /* Beacon settings */

    /* Antenna Deployment settings */
    uint16_t antenna_deploy_delay_s;
    bool antenna_deploy_enabled;
    uint8_t antenna_deploy_tries_counter;

    /* extra functionality */
    bool APRS_eanbled;
} nvram_cdh_t;

retval_t CDH_delay_beacon(uint32_t seconds);
bool CDH_command_is_internal(frame_t *cmd);
retval_t CDH_command_new(frame_t **cmd_frame, uint32_t sequence_number, uint8_t ss, uint8_t cmd);
retval_t CDH_command_enqueue(frame_t *cmd_frame);
retval_t CDH_command_enqueue_delayed(uint32_t delay_s, frame_t *cmd);
void CDH_command_watchdog_kick(void);
retval_t CDH_process_hard_commands(frame_t *cmd_frame);
bool CDH_antenna_is_deployment_enabled();

/* cdh/sampler.c (used by AOCS) */
void cdh_sample_init(void);
retval_t cdh_sample_beacon_tick(frame_t *beacon);
retval_t cdh_sample_memory_tick(void);
retval_t cmd_sample_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_sample_memory(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_sample_flash_when_done(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_sample_retrieve(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

#endif
