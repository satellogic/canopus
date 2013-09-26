#include <canopus/types.h>
#include <canopus/drivers/commhub_1500.h>

#include <FreeRTOS.h>
#include <task.h>

#define INVALID_SWITCHES_STATE	0xFFFF
#define SWITCHES_DEFAULT_STATE 0x0000 // This is an empiric measure, shift registers doesn't have an initial state defined.
static uint16_t shifted_switches_current_state = SWITCHES_DEFAULT_STATE;

static retval_t shifter_out_bit(uint16_t value) {
	retval_t rv;
	value = value?COMMHUB_GP_OUT_SHR_DATA:0;

	rv = commhub_writeRegisterNoSync(COMMHUB_REG_GP_OUT_SHIFTER, value);
	SUCCESS_OR_RETURN(rv);

	vTaskDelay(1);
	rv = commhub_writeRegisterNoSync(COMMHUB_REG_GP_OUT_SHIFTER, value + COMMHUB_GP_OUT_SHR_CLK);
	SUCCESS_OR_RETURN(rv);

	vTaskDelay(1);
	return commhub_writeRegisterNoSync(COMMHUB_REG_GP_OUT_SHIFTER, value);
}

static retval_t shifter_latch_down() {
	return commhub_writeRegisterNoSync(COMMHUB_REG_GP_OUT_SHIFTER, 0);
}

static retval_t shifter_latch_up() {
	return commhub_writeRegisterNoSync(COMMHUB_REG_GP_OUT_SHIFTER, COMMHUB_GP_OUT_SHR_LATCH);
}

static retval_t shift_out(uint16_t value) {
	retval_t rv;
	int i;
	rv = commhub_sync();
	SUCCESS_OR_RETURN(rv);

	rv = shifter_latch_down();
	SUCCESS_OR_RETURN(rv);

	for (i=0;i<16;i++) {
		rv = shifter_out_bit(value & 1);
		SUCCESS_OR_RETURN(rv);

		value >>= 1;
	}
	vTaskDelay(1);
	rv = shifter_latch_up();
	SUCCESS_OR_RETURN(rv);

	vTaskDelay(1);
	return shifter_latch_down();
}

retval_t shifted_setAll(uint16_t switches) {
	retval_t rv;
	rv = shift_out(switches);

	if (RV_SUCCESS == rv) {
		shifted_switches_current_state = switches;
	} else {
		shifted_switches_current_state = INVALID_SWITCHES_STATE;
	}
	return rv;
}

retval_t shifted_getAll(uint16_t *switches) {
	retval_t rv;

	if (shifted_switches_current_state != INVALID_SWITCHES_STATE) {
		*switches = shifted_switches_current_state;
		rv = RV_SUCCESS;
	} else {
		rv = RV_ERROR;
	}
	return rv;
}

retval_t shifted_turnOn(uint16_t bitmask) {
	retval_t rv;

	if (shifted_switches_current_state != INVALID_SWITCHES_STATE) {
		rv = shifted_setAll(shifted_switches_current_state | bitmask);
	} else {
		rv = RV_ERROR;
	}
	return rv;
}

retval_t shifted_turnOff(uint16_t bitmask) {
	retval_t rv;

	if (shifted_switches_current_state != INVALID_SWITCHES_STATE) {
		rv = shifted_setAll(shifted_switches_current_state & ~bitmask);
	} else {
		rv = RV_ERROR;
	}
	return rv;
}

retval_t shifter_initialize(){
	return shifted_setAll(SWITCHES_DEFAULT_STATE);
}
