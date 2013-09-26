#include <canopus/types.h>
#include <canopus/assert.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/imu/adis16xxx.h>

#include <FreeRTOS.h>
#include <task.h>

#define CHECK_OR_RET(__rv) if (RV_SUCCESS != (__rv)) return (__rv);

#undef IMU_DRIVER_DEBUG
//#define IMU_DRIVER_DEBUG_CHANNEL_TRANSACT yes
#undef IMU_DRIVER_DEBUG_CHANNEL_TRANSACT

retval_t adis1640x_read_register(const channel_t *const channel, uint8_t reg_nr, uint16_t * const value)
{
	frame_t cmd1    = DECLARE_FRAME_BYTES(reg_nr, 0);
	frame_t cmd2    = DECLARE_FRAME_BYTES(0, 0);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv;

	assert(reg_nr <= 0x57);

#ifdef IMU_DRIVER_DEBUG
	printf("read: %x %x\n", reg_nr, reg_nr + 1);
#endif

	// TODO: this is splitted in two transacts because the delay between two
	// words (in TMS driver) is smaller than 9us as specified on datasheet.
	// Slow VCLK1 and join the transacts
	rv = channel_transact(channel, &cmd1, 0, &answer);

#ifdef IMU_DRIVER_DEBUG_CHANNEL_TRANSACT
	printf("CHANNEL_TRANSACT_RV is %d\n",rv);
#endif
	CHECK_OR_RET(rv);

	vTaskDelay(1);
	frame_reset(&answer);
	rv = channel_transact(channel, &cmd2, 0, &answer);
	CHECK_OR_RET(rv);

	frame_reset(&answer);
	rv = frame_get_u16(&answer, value);

#ifdef IMU_DRIVER_DEBUG
	printf("value: %x %x\n", HI8(*value), LO8(*value));
#endif

	return rv;
}

retval_t adis1640x_write_register(const channel_t *const channel, uint8_t reg_nr, uint16_t value)
{
	frame_t cmd1    = DECLARE_FRAME_BYTES(ADIS16400_WRITE_REG(reg_nr)    , LO8(value));
	frame_t cmd2    = DECLARE_FRAME_BYTES(ADIS16400_WRITE_REG(reg_nr + 1), HI8(value));
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv = RV_ERROR;

	assert(reg_nr <= 0x57);

#ifdef IMU_DRIVER_DEBUG
	printf("write: %x %x, %x %x\n",
			ADIS16400_WRITE_REG(reg_nr),
			ADIS16400_WRITE_REG(reg_nr + 1), HI8(value), LO8(value));
#endif

	// TODO: this is splitted in two transacts because the delay between two
	// words (in TMS driver) is smaller than 9us as specified on datasheet.
	// Slow VCLK1 and join the transacts
	rv = channel_transact(channel, &cmd1, 0, &answer);
	CHECK_OR_RET(rv);


	vTaskDelay(1);
	frame_reset(&answer);
	rv = channel_transact(channel, &cmd2, 0, &answer);

#ifdef IMU_DRIVER_DEBUG_CHANNEL_TRANSACT
	printf("CHANNEL_TRANSACT_RV is %d\n",rv);
#endif

	return rv;
}

#define CHECK(__rv) if (__rv!=RV_SUCCESS) return __rv;

retval_t adis1640x_read_burst(const channel_t *const channel, adis1640x_data_burst_raw * const value)
{
        frame_t cmd    = DECLARE_FRAME_BYTES(ADIS1640x_BURST_CMD, 0, ADIS1640x_BURST_0s);
        frame_t answer = DECLARE_FRAME_SPACE(2 + 2*12);
        retval_t rv;
        uint16_t nada;

        rv = channel_transact(channel, &cmd, 0, &answer);

#ifdef IMU_DRIVER_DEBUG_CHANNEL_TRANSACT
	printf("CHANNEL_TRANSACT_RV is %d\n",rv);
#endif

        CHECK(rv);

        frame_reset(&answer);
        rv = frame_get_u16(&answer, &nada);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->power_supply_voltage);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->gyro_x);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->gyro_y);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->gyro_z);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->accel_x);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->accel_y);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->accel_z);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->mag_x);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->mag_y);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->mag_z);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->temp);
        CHECK(rv);
        rv = frame_get_u16(&answer, &value->adc);
        CHECK(rv);

        return RV_SUCCESS;
}

