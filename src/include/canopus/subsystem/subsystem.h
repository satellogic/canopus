#ifndef _CANOPUS_SUBSYSTEM_H_
#define _CANOPUS_SUBSYSTEM_H_

#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/subsystem/command.h>
#include <cmockery.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#define MATRIX_KEY_INVALID  -1
#define MATRIX_KEY_ENABLED  0
#define MATRIX_KEY_DISABLED 0xbaddca11

/* Priorities for the tasks. */
#define TASK_PRIORITY_SUBSYSTEM_PLATFORM	( tskIDLE_PRIORITY + 4 )
#define TASK_PRIORITY_SUBSYSTEM_MEMORY		( tskIDLE_PRIORITY + 3 )
#define TASK_PRIORITY_SUBSYSTEM_POWER		( tskIDLE_PRIORITY + 3 )
#define TASK_PRIORITY_SUBSYSTEM_THERMAL		( tskIDLE_PRIORITY + 2 )
#define TASK_PRIORITY_SUBSYSTEM_CDH			( tskIDLE_PRIORITY + 2 )
#define TASK_PRIORITY_SUBSYSTEM_AOCS		( tskIDLE_PRIORITY + 2 )
#define TASK_PRIORITY_SUBSYSTEM_PAYLOAD		( tskIDLE_PRIORITY + 2 )

#define TASK_PRIORITY_RADIO					TASK_PRIORITY_SUBSYSTEM_CDH

/* default stack depths for each subsystem's task */
#define SUBSYSTEM_MINIMAL_STACKSIZE 1024
#define STACK_DEPTH_PLATFORM    SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_CDH         SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_AOCS        SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_MEMORY      SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_POWER       SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_PAYLOAD     SUBSYSTEM_MINIMAL_STACKSIZE
#define STACK_DEPTH_THERMAL     SUBSYSTEM_MINIMAL_STACKSIZE

#define METHOD(x) typeof(x) *x

/** Enumrates all the possible modes of the satellite **/
typedef enum satellite_mode_e {
	/* Values in synch with power_matrix in POWER (pms.c) */
	SM_OFF          = 0,
    SM_BOOTING      = 1,
    SM_INITIALIZING = 2,
    SM_SURVIVAL     = 3,
    SM_MISSION      = 4,
    SM_LOW_POWER    = 5,
   /* WARNING !!! WARNING !!! when adding modes one must modify the
    * SATELLITE_MODE_WITH_AOCS macro in aocs.h
    */

    SM_COUNT
} satellite_mode_e;

/** Enumrates all subsystems in the satellite **/
typedef enum ss_id_e {
	/* This enum sets the subsystem initialization order */
    SS_PLATFORM = 0,     /**< plataform manager */
    SS_MEMORY,
    SS_CDH,        /**< command and data handling */
    SS_POWER,
    SS_THERMAL,
    SS_AOCS,       /**< attitude determination and control */
    SS_PAYLOAD,
    SS_TEST,

    SS_FUTURE_1,
    SS_FUTURE_2,
    SS_FUTURE_3,
    SS_FUTURE_4,
    SS_FUTURE_5,

    SS_MAX
} ss_id_e;

typedef enum ss_st_e {
	SS_ST_BOOTING,
	SS_ST_READY,
	SS_ST_MODE_CHANGE_PENDING,
	SS_ST_FATAL_ERROR,
} ss_st_e;

struct subsystem_t;
struct subsystem_api_t;
struct subsystem_config_t;
struct subsystem_state_t;

/** subsystem_state_t
 * Mutable structure holding the current state for a subsystem
 **/
typedef struct subsystem_state_t {
    xTaskHandle task_handle;
    xSemaphoreHandle semphr;
    ss_st_e status;
    uint8_t current_heartbeats;
    uint8_t expected_heartbeats;
    void *status_arg_p;
    uint32_t status_arg_u32;
} subsystem_state_t;

#define HEARTBEAT_MAIN_TASK		(1 << 0)
#define HEARTBEAT_2nd_TASK		(1 << 1)
#define HEARTBEAT_3rd_TASK		(1 << 2)
#define HEARTBEAT_4th_TASK		(1 << 3)
#define HEARTBEAT_5th_TASK		(1 << 4)
#define HEARTBEAT_6th_TASK		(1 << 5)
#define HEARTBEAT_7th_TASK		(1 << 6)
#define HEARTBEAT_8th_TASK		(1 << 7)

typedef portTASK_FUNCTION(ss_main_task_t, ss_main_context);
typedef retval_t ss_command_execute_t(const struct subsystem_t *self, frame_t *frame_in, frame_t *frame_out, uint32_t sequence_number);
typedef retval_t ss_initialize_t(const struct subsystem_t *self);

typedef const struct subsystem_api_t {
    ss_main_task_t *main_task;
    ss_command_execute_t *command_execute;
} subsystem_api_t;

typedef struct ss_tests_t {
	const UnitTest *tests;
	size_t count;
} ss_tests_t;

typedef const struct subsystem_config {
    unsigned portBASE_TYPE uxPriority; /**< task priority */
    unsigned portSHORT usStackDepth;   /**< task stack depth */
    const ss_id_e id;         
    const char * name;
    const ss_tests_t *tests;
    const ss_command_handler_t *command_handlers;
    size_t command_handlers_count;
} subsystem_config_t;

#define DECLARE_COMMAND_HANDLERS(__command_handlers)	\
	.command_handlers		= __command_handlers,		\
	.command_handlers_count = ARRAY_COUNT(__command_handlers)	\

/** Instance of a subsystem. **/
typedef const struct subsystem_t {
    subsystem_api_t  * const api;
    subsystem_config_t * const config;
    subsystem_state_t *state;
} subsystem_t;

typedef enum { SS_CRITICAL_ERR, SS_FATAL_ERR } ss_error_e;
#define SS_CRITICAL_ERROR(__ss)     ss_report_error(__ss, SS_CRITICAL_ERR, __FILE__, __LINE__)
#define SS_FATAL_ERROR(__ss)        ss_report_error(__ss, SS_FATAL_ERR, __FILE__, __LINE__)

extern const subsystem_t *const subsystems[SS_MAX];

retval_t ss_command_execute(const struct subsystem_t * self, frame_t * iframe, frame_t * oframe, uint32_t sequence_number);
void ss_report_error(const struct subsystem_t *ss, ss_error_e error, char *filename, uint32_t line);

bool board_enabled_subsystem_id(ss_id_e id);

int SUBSYSTEM_run_tests(const subsystem_t *ss, uint32_t test_number);
#endif
