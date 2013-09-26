#include <cmockery.h>
#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/commhub_1500.h>

static void test_commhub_sync(void **s) {
	retval_t rv;

	rv = commhub_sync();
	assert_int_equal(RV_SUCCESS, rv);
}

static void test_commhub_read_constant(void **s) {
	retval_t rv;
	uint16_t data;

	rv = commhub_readRegister(COMMHUB_REG_SYNC, &data);
	assert_int_equal(RV_SUCCESS, rv);
	assert_int_equal(F_SYNC_RESPONSE, data);
}

static void test_commhub_write(void **s) {
	retval_t rv;
	uint16_t data1, data2;

	rv = commhub_readRegister(COMMHUB_REG_UART_OVERO_RX, &data1);
	assert_int_equal(RV_SUCCESS, rv);
	rv = commhub_andRegister(COMMHUB_REG_UART_OVERO_RX, 0x000F);
	assert_int_equal(RV_SUCCESS, rv);
	rv = commhub_readRegister(COMMHUB_REG_UART_OVERO_RX, &data2);
	assert_int_equal(RV_SUCCESS, rv);

	assert_int_equal(data1 & 0x000F, data2);
	rv = commhub_orRegister(COMMHUB_REG_UART_OVERO_RX, 0x5A50);
	assert_int_equal(RV_SUCCESS, rv);

	rv = commhub_readRegister(COMMHUB_REG_UART_OVERO_RX, &data2);
	assert_int_equal(RV_SUCCESS, rv);
	assert_int_equal((data1 & 0x000F) | 0x5A50, data2);
}

static const UnitTest tests[] = {
	unit_test(test_commhub_sync),
    unit_test(test_commhub_read_constant),
    unit_test(test_commhub_write),
};

const ss_tests_t platform_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};
