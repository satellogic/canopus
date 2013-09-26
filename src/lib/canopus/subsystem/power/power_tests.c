#include <cmockery.h>
#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/drivers/power/eps.h>
#include <canopus/drivers/power/ina209.h>
#include <canopus/board/channels.h>

static void test_EPS_BatteryV_in_range(void **s) {
	uint16_t battery_v_raw;
	float battery_v;
	retval_t rv;

	rv = eps_GetADCRaw(EPS_ADC_BATTERY_v, &battery_v_raw);
	assert_int_equal(rv, RV_SUCCESS);

	battery_v = eps_ADC_convertion(EPS_ADC_BATTERY_v, battery_v_raw);

	assert_true(battery_v >= 6.35);
	assert_true(battery_v <= 8.45);
}

static void test_EPS_firmwareVersionAsExpected(void **s) {
	uint16_t firmware_version;
	retval_t rv;

	rv = eps_GetFirmwareVersion(&firmware_version);
	assert_int_equal(rv, RV_SUCCESS);
	assert_int_equal(firmware_version, EPS_FIRMWARE_VERSION);
}

static void test_INA_pd3v3_id(void **s) {
	retval_t rv;

	rv = ina209_init(ch_ina_pd3v3);
	assert_int_equal(rv, RV_SUCCESS);
}

static void test_INA_pd5v_id(void **s) {
	retval_t rv;

	rv = ina209_init(ch_ina_pd5v);
	assert_int_equal(rv, RV_SUCCESS);
}

static void test_INA_pd12v_id(void **s) {
	retval_t rv;

	rv = ina209_init(ch_ina_pd12v);
	assert_int_equal(rv, RV_SUCCESS);
}

//static void test_fpga_register(void **s) {
//	retval_t rv;
//	uint16_t value;
//
//	rv = fpga_writeRegister(REG_MEM_0, 0xA55A);
//	assert_int_equal(RV_SUCCESS, rv);
//
//	value = 0;
//	rv = fpga_readRegister(REG_MEM_0, &value);
//	assert_int_equal(RV_SUCCESS, rv);
//	assert_int_equal(0xA55A, value);
//}

static const UnitTest tests[] = {
    unit_test(test_EPS_BatteryV_in_range),
    unit_test(test_EPS_firmwareVersionAsExpected),
    unit_test(test_INA_pd3v3_id),
    unit_test(test_INA_pd5v_id),
    unit_test(test_INA_pd12v_id),
};

const ss_tests_t power_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};
