#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/imu/adis16xxx.h>
#include <cmockery.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/subsystem/aocs/css.h>
#include <canopus/subsystem/aocs/pointing.h>
#include <canopus/logging.h>
#include <canopus/board/adc.h>

static void test_adis16400_id(void **s) {
	uint16_t device_id;
	retval_t rv;

	rv = adis1640x_read_register(ch_adis, ADIS16400_PRODUCT_ID, &device_id);
	assert_int_equal(RV_SUCCESS, rv);

	assert_true((IMU16400_DEVICE_ID_ENG == device_id || IMU16400_DEVICE_ID_FLIGHT == device_id));
}

static void test_read_write(void **state) {
	uint16_t value_orig, value;
	retval_t rv;

	/* Read a register, save it, change it, restore it */
	rv = adis1640x_read_register(ch_adis, ADIS16400_XACCL_OFF, &value_orig);
	assert_int_equal(RV_SUCCESS, rv);

	rv = adis1640x_write_register(ch_adis, ADIS16400_XACCL_OFF, 0x075a);
	assert_int_equal(RV_SUCCESS, rv);

	rv = adis1640x_read_register(ch_adis, ADIS16400_XACCL_OFF, &value);
	assert_int_equal(RV_SUCCESS, rv);
	assert_int_equal(0x075a, value);

	rv = adis1640x_write_register(ch_adis, ADIS16400_XACCL_OFF, value_orig);
	rv = adis1640x_read_register(ch_adis, ADIS16400_XACCL_OFF, &value);
	assert_int_equal(RV_SUCCESS, rv);
	assert_int_equal(value_orig, value);
}

static void test_self_test(void **state) {
	uint16_t diag_stat;
	uint16_t msc_ctrl;
	retval_t rv;

	rv = adis1640x_read_register(ch_adis, ADIS16400_MSC_CTRL, &msc_ctrl);
	assert_int_equal(RV_SUCCESS, rv);

	rv = adis1640x_write_register(ch_adis, ADIS16400_MSC_CTRL, msc_ctrl | ADIS16400_MSC_CTRL_INT_SELF_TEST);
	assert_int_equal(RV_SUCCESS, rv);

	vTaskDelay(portTICK_RATE_MS / ADIS16400_SELF_TEST_DELAY);

	rv = adis1640x_read_register(ch_adis, ADIS16400_DIAG_STAT, &diag_stat);
	assert_int_equal(RV_SUCCESS, rv);
	assert_int_equal(0, diag_stat);
}

#define ASUNTEST(__testnum)	do {\
volts2sunvec(res, test##__testnum##_pos, test##__testnum##_neg);\
for(i = 0; i < 3; i++)\
assert_in_range(res[i], res##__testnum[i] - eps, res##__testnum[i] + eps);\
}while(0);\

static void test_sun_vector(void **state) {
	int i = 0;
	double eps = 1./100.;

	vectord_t test1_pos = {2.7, 0, 0};
	vectord_t test1_neg = {0, 0, 0};
	vectord_t res1 = {1, 0, 0};

	vectord_t test2_pos = {0, 2.7, 0};
	vectord_t test2_neg = {0, 0, 0};
	vectord_t res2 = {0, 1, 0};

	vectord_t test3_pos = {0, 0, 2.7};
	vectord_t test3_neg = {0, 0, 0};
	vectord_t res3 = {0, 0, 1};

	vectord_t test4_pos = {0, 0, 0};
	vectord_t test4_neg = {2.7, 0, 0};
	vectord_t res4 = {-1, 0, 0};

	vectord_t test5_pos = {0, 0, 0};
	vectord_t test5_neg = {0, 2.7, 0};
	vectord_t res5 = {0, -1, 0};

	vectord_t test6_pos = {0, 0, 0};
	vectord_t test6_neg = {0, 0, 2.7};
	vectord_t res6 = {0, 0, -1};

	vectord_t test7_pos = {2, 2, 2};
	vectord_t test7_neg = {0, 0, 0};
	vectord_t res7 = {sqrt(3), sqrt(3), sqrt(3)};

	vectord_t test8_pos = {0, 0, 0};
	vectord_t test8_neg = {2, 2, 2};
	vectord_t res8 = {-sqrt(3), -sqrt(3), -sqrt(3)};

	vectord_t res;

	ASUNTEST(1)
	ASUNTEST(2)
	ASUNTEST(3)
	ASUNTEST(4)
	ASUNTEST(5)
	ASUNTEST(6)
	ASUNTEST(7)
	ASUNTEST(8)
}

static void test_read_burst(void **state) {
       adis1640x_data_burst_raw data_out;
       retval_t rv;

       double norm, accel_x_g, accel_y_g, accel_z_g, gyro_x_dps, gyro_y_dps, gyro_z_dps;
       double power_voltage_v;

       rv = adis1640x_read_burst(ch_adis, &data_out);
       assert_int_equal(RV_SUCCESS, rv);

       /* 5V +/- .25V (same as self test)
        * 0x814 = 5V, then 1 bit = 2.418 mV (table 9 ADIS16400/ADIS16405 rev. B)
        */
       power_voltage_v = RAW2LSB14(data_out.power_supply_voltage) * 2.418;
       assert_in_range(power_voltage_v, 5000. - 250., 5000. + 250.);

       /* 0 dps +/- 1 dps
        * 1 bit = 0.0125 dps (table 9 and 17 ADIS16400/ADIS16405 rev. B)
        * Milleage may vary with different SENS_AVGs, but all of them should usually pass this test.
        */
       gyro_x_dps = RAW2LSB14(data_out.gyro_x) * 0.0125;
       gyro_y_dps = RAW2LSB14(data_out.gyro_y) * 0.0125;
       gyro_z_dps = RAW2LSB14(data_out.gyro_z) * 0.0125;
       assert_in_range(gyro_x_dps, -1., 1.);
       assert_in_range(gyro_y_dps, -1., 1.);
       assert_in_range(gyro_z_dps, -1., 1.);

       /* Table 9 ADIS16400/ADIS16405 rev. B */
       accel_x_g = RAW2LSB14(data_out.accel_x) * 3.33;
       accel_y_g = RAW2LSB14(data_out.accel_y) * 3.33;
       accel_z_g = RAW2LSB14(data_out.accel_z) * 3.33;

       norm = sqrt(pow(accel_x_g,2) + pow(accel_y_g,2) + pow(accel_z_g,2));

       assert_in_range(norm, 1000. - 250., 1000. + 250.);
}

static void test_pointing(void **state) {
	matrixf_t I = MATRIXF_IDENTITY;
	matrixf_t x180 = {{1.,0,0},{0, -1., 0},{0,0,-1}};
	matrixf_t y180 = {{-1.,0,0},{0, 1., 0},{0,0,-1}};
	matrixf_t z180 = {{-1.,0,0},{0, -1., 0},{0,0,1}};
	vectorf_t qv;
	float qs;
	/***************/
	vectorf_t b1 = {0,0,1.}, i1 = {0,0,1.};
	vectorf_t b2 = {0,2.,0}, i2 = {0,2.,0};
	matrixf_t A;


	rotmat2quat(I, qv, &qs);
	assert_true(qv[0] == 0 && qv[1] == 0 && qv[2] == 0 && qs == 1);

	rotmat2quat(x180, qv, &qs);
	assert_true(qv[0] == 1 && qv[1] == 0 && qv[2] == 0 && qs == 0);

	rotmat2quat(y180, qv, &qs);
	assert_true(qv[0] == 0 && qv[1] == 1 && qv[2] == 0 && qs == 0);

	rotmat2quat(z180, qv, &qs);
	assert_true(qv[0] == 0 && qv[1] == 0 && qv[2] == 1 && qs == 0);

	/***************/
	triad(b1, i1, b2, i2, A);
	assert_true(A[0][0] == 1 && A[0][1] == 0 && A[0][2] == 0);
	assert_true(A[1][0] == 0 && A[1][1] == 1 && A[1][2] == 0);
	assert_true(A[2][0] == 0 && A[2][1] == 0 && A[2][2] == 1);
}

static const UnitTest tests[] = {
    unit_test(test_adis16400_id),
    unit_test(test_read_write),
    unit_test(test_self_test),
    unit_test(test_sun_vector),
    unit_test(test_read_burst),
    unit_test(test_pointing),
//    unit_test(test_the_adc),
};

const ss_tests_t aocs_tests = {
		.tests = tests,
		.count = (sizeof(tests)/sizeof(tests[0]))
};

