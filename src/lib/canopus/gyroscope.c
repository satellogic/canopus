#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/gyroscope.h>
#include <stddef.h>

retval_t gyroscope_initialize(const gyroscope_t * const gyroscope,
                              const gyroscope_config_t * const config)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->initialize(gyroscope, config);
}


retval_t gyroscope_deinitialize(const gyroscope_t * const gyroscope)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->deinitialize(gyroscope);
}

retval_t gyroscope_get_temp(const gyroscope_t *gyroscope,
                            float * temp_out_C,
                            timespec_t *timestamp)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->get_temp(gyroscope, temp_out_C, timestamp);
}

retval_t gyroscope_get_xyz(const gyroscope_t * const gyroscope,
                           float * const xout,
                           float * const yout,
                           float * const zout,
                           timespec_t * const timestamp)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->get_xyz(gyroscope,
                                    xout, yout, zout,
                                    timestamp);
}

retval_t gyroscope_get_xyz_raw(const gyroscope_t * const gyroscope,
                           int16_t * const xout,
                           int16_t * const yout,
                           int16_t * const zout,
                           timespec_t * const timestamp)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->get_xyz_raw(gyroscope,
                                    xout, yout, zout,
                                    timestamp);
}

retval_t gyroscope_compensate_temp_drift(
            const gyroscope_t *gyroscope)
{
    ENGINEERING_ASSERT(gyroscope != NULL);
	if(NULL == gyroscope) return RV_ILLEGAL;
    ENGINEERING_ASSERT(gyroscope->base != NULL);
	if(NULL == gyroscope->base) return RV_ILLEGAL;
    return gyroscope->base->compensate_temp_drift(gyroscope);
}

#ifdef GYROSCOPE_DEMO

#include <canopus/logging.h>

/* Demo */
retval_t gyroscope_demo(const gyroscope_t * const gyro)
{
    retval_t rv;
    float xout, yout, zout;

    Console_report_fmt("[  Gyroscope demo: #<%p>\r\n", gyro);

    Console_report("|> gyroscope_get_xyz() (values scaled up by 1000)\r\n");
    rv = gyroscope_get_xyz(gyro, &xout, &yout, &zout, NULL);
    if (RV_SUCCESS != rv)
            Console_report_fmt("|< ERROR: %d\r\n", rv);
    else Console_report_fmt("|< OK: x=%d, y=%d, z=%d\r\n",(int)(xout * 1000.0),
                                                          (int)(yout * 1000.0),
                                                          (int)(zout * 1000.0));

    Console_report_fmt("] Gyroscope demo. #<%p>\r\n", gyro);

    if (RV_SUCCESS != rv)
        return RV_ERROR;
    return rv;
}

#endif /* GYROSCOPE_DEMO */
