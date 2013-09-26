#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/drivers/radio/lithium.h>
#include <cmockery.h>

static void test_lithium_op_counter_increments(void **s) {
    int op_counter;
    lithium_telemetry_t telemetry;
	retval_t rv;

    rv = lithium_get_telemetry(&telemetry);
    assert_int_equal(RV_SUCCESS, rv);

    op_counter = telemetry.op_counter;

    rv = lithium_get_telemetry(&telemetry);
    assert_int_equal(RV_SUCCESS, rv);

    assert_true(op_counter < telemetry.op_counter);
}

static const UnitTest tests[] = {
    unit_test(test_lithium_op_counter_increments),
};

const ss_tests_t cdh_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};
