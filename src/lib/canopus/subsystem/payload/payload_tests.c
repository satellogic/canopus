#include <cmockery.h>
#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/board/channels.h>

static const UnitTest tests[] = {
};

const ss_tests_t payload_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};
