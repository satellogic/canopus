#include <cmockery.h>
#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>

static void test_tests_ok(void **s) {
	assert_true(true);
	assert_false(false);
	assert_int_equal(1,2/2);
}

static void test_tests_failed(void **s) {
	assert_true(false);
}

static const UnitTest tests[] = {
    unit_test(test_tests_ok),
    unit_test(test_tests_failed),
};

const ss_tests_t test_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};

