#include <canopus/logging.h>
#include <canopus/drivers/commhub_1500.h>
#include <canopus/subsystem/power.h>
#include <canopus/nvram.h>

#define ANTENNA_BURNER_ARMS_A_C_0	POWER_SW_ANTENNA_DEPLOY_1
#define ANTENNA_BURNER_ARMS_A_C_1	POWER_SW_ANTENNA_DEPLOY_3
#define ANTENNA_BURNER_ARMS_B_D_0	POWER_SW_ANTENNA_DEPLOY_2
#define ANTENNA_BURNER_ARMS_B_D_1	POWER_SW_ANTENNA_DEPLOY_4

#define ANTENNA_SENSOR_ARM_A		COMMHUB_GP_IN_ANT_SW_1
#define ANTENNA_SENSOR_ARM_B		COMMHUB_GP_IN_ANT_SW_2
#define ANTENNA_SENSOR_ARM_C		COMMHUB_GP_IN_ANT_SW_3
#define ANTENNA_SENSOR_ARM_D		COMMHUB_GP_IN_ANT_SW_4

#define ANTENNA_DEPLOY_ABSOLUTE_MAX_TIME_s	30

bool CDH_antenna_is_deployment_enabled() {
	return nvram.cdh.antenna_deploy_enabled;
}

static void antenna_inhibit_further_deployment() {
	if (nvram.cdh.antenna_deploy_enabled) {
		nvram.cdh.antenna_deploy_enabled = false;
		MEMORY_nvram_save(&nvram.cdh.antenna_deploy_enabled, sizeof nvram.cdh.antenna_deploy_enabled);
	}
}

static int32_t antenna_deploy_tries_next_or_disable() {
	if (CDH_ANTENNA_DEPLOY_MAX_TRIES == nvram.cdh.antenna_deploy_tries_counter) {
		antenna_inhibit_further_deployment();
		return -1;
	} else {
		nvram.cdh.antenna_deploy_tries_counter++;
		MEMORY_nvram_save(&nvram.cdh.antenna_deploy_tries_counter, sizeof nvram.cdh.antenna_deploy_tries_counter);
		return nvram.cdh.antenna_deploy_tries_counter-1;
	}
}

static enum {ANTENNA_KEEP_WAITING, ANTENNA_DEPLOY_NOW} antenna_wait_for_deployment() {
	int elapsed_s;
	int delay_s;

	if (0 == nvram.cdh.antenna_deploy_tries_counter) {
		delay_s = ANTENNA_DEPLOY_INITIAL_DELAY_s;
	} else {
		delay_s = nvram.cdh.antenna_deploy_delay_s;
	}

	for (elapsed_s=0; elapsed_s < delay_s; elapsed_s += CDH_ANTENNA_DEPLOY_DELAY_INTERVAL_s) {
		if (!nvram.cdh.antenna_deploy_enabled) {
			log_report_fmt(LOG_SS_CDH_ANTENNA, "ANTENNA: Deploy inhibited. Waiting 5 minutes\n");
			vTaskDelay(5*60 * 1000 / portTICK_RATE_MS);
			return ANTENNA_KEEP_WAITING;
		}

		log_report_fmt(LOG_SS_CDH_ANTENNA, "ANTENNA: Deploying in %d seconds. Use 'cdh antenna_deploy_inhibit 1' to stop.\n", delay_s - elapsed_s);
		vTaskDelay(CDH_ANTENNA_DEPLOY_DELAY_INTERVAL_s * 1000 / portTICK_RATE_MS);
	}

	return ANTENNA_DEPLOY_NOW;
}

static void antenna_deploy(power_switch_e burner) {
	int i;

	for (i=0;i<5;i++) {
		if (RV_SUCCESS == POWER_turnOn(burner)) break;
		vTaskDelay(777 / portTICK_RATE_MS);
	}

	vTaskDelay(ANTENNA_DEPLOY_ABSOLUTE_MAX_TIME_s * 1000 / portTICK_RATE_MS);

	for (i=0;i<5;i++) {
		if (RV_SUCCESS == POWER_turnOff(burner)) break;
		vTaskDelay(777 / portTICK_RATE_MS);
	}
}

void antenna_deploy_task(void *_ss) {
	POWER_turnOff(ANTENNA_BURNER_ARMS_A_C_0);
	POWER_turnOff(ANTENNA_BURNER_ARMS_A_C_1);
	POWER_turnOff(ANTENNA_BURNER_ARMS_B_D_0);
	POWER_turnOff(ANTENNA_BURNER_ARMS_B_D_1);

	while (1) {
		if (ANTENNA_KEEP_WAITING == antenna_wait_for_deployment()) {
			continue;
		}

		if (antenna_deploy_tries_next_or_disable() % 2) {
			antenna_deploy(ANTENNA_BURNER_ARMS_A_C_0);
			antenna_deploy(ANTENNA_BURNER_ARMS_B_D_0);
		} else {
			antenna_deploy(ANTENNA_BURNER_ARMS_B_D_1);
			antenna_deploy(ANTENNA_BURNER_ARMS_A_C_1);
		}
	}
}

retval_t antenna_get_sensors(uint8_t *sensors) {
	retval_t rv;
	uint16_t _sensors;
	rv = commhub_readRegister(COMMHUB_REG_GP_IN, &_sensors);
	_sensors &= ANTENNA_SENSOR_ARM_A | ANTENNA_SENSOR_ARM_B | ANTENNA_SENSOR_ARM_C | ANTENNA_SENSOR_ARM_D;
	*sensors = _sensors;
	return rv;
}
