#include <canopus/assert.h>
#include <canopus/drivers/simusat/gyroscope.h>
#include <stddef.h>

static retval_t _get_temp(const gyroscope_t * const gyroscope,
                  float * const temp,
                  timespec_t * const timestamp)
{
    posix_gyroscope_state_t * g_state;
    assert(gyroscope != NULL);

    g_state = (posix_gyroscope_state_t*)gyroscope->state;
    if (!g_state->is_initialized) return RV_ILLEGAL;

    if (temp != NULL) *temp = -69.42;

    /* TODO fill timestamp */
    return RV_SUCCESS;
}

static retval_t _get_xyz(const gyroscope_t * const gyroscope,
                  float * const xout,
                  float * const yout,
                  float * const zout,
                  timespec_t * const timestamp)
{
    posix_gyroscope_state_t * g_state;
    assert(gyroscope != NULL);


    g_state = (posix_gyroscope_state_t*)gyroscope->state;
    if (!g_state->is_initialized) return RV_ILLEGAL;

    /* Currently using arbitrary values for x, y & z. */
    if (xout != NULL) *xout = .06;
    if (yout != NULL) *yout = .1;
    if (zout != NULL) *zout = .04;

    /* TODO fill timestamp */
    return RV_SUCCESS;
}

static retval_t _get_xyz_raw(const gyroscope_t * const gyroscope,
                  int16_t * const xout,
                  int16_t * const yout,
                  int16_t * const zout,
                  timespec_t * const timestamp)
{
    posix_gyroscope_state_t * g_state;
    assert(gyroscope != NULL);

    g_state = (posix_gyroscope_state_t*)gyroscope->state;
    if (!g_state->is_initialized) return RV_ILLEGAL;

    /* Currently using arbitrary values for x, y & z. */
    if (xout != NULL) *xout = 0xAABB;
    if (yout != NULL) *yout = 0xCCDD;
    if (zout != NULL) *zout = 0xEEFF;

    /* TODO fill timestamp */
    return RV_SUCCESS;
}

static retval_t _initialize(const gyroscope_t * const gyroscope,
                     const gyroscope_config_t * const config)
{
    posix_gyroscope_state_t * g_state;

    assert(gyroscope != NULL);
    assert(gyroscope->state != NULL);
    assert(config != NULL);

    g_state = (posix_gyroscope_state_t*)gyroscope->state;
    if (g_state->is_initialized) return RV_ILLEGAL;

    g_state->is_initialized = true;
    return RV_SUCCESS;
}

static retval_t _deinitialize(const gyroscope_t * const gyroscope)
{
    posix_gyroscope_state_t * g_state;

    assert(gyroscope != NULL);
    assert(gyroscope->state != NULL);

    g_state = (posix_gyroscope_state_t*)gyroscope->state;
    if (!g_state->is_initialized) return RV_ILLEGAL;

    g_state->is_initialized = false;
    return RV_SUCCESS;
}

static retval_t _compensate_temp_drift(const gyroscope_t * const gyroscope)
{
    return RV_SUCCESS;
}

const gyroscope_base_t posix_gyroscope_base = {
    .initialize   = &_initialize,
    .deinitialize = &_deinitialize,
    .get_xyz      = &_get_xyz,
    .get_xyz_raw      = &_get_xyz_raw,
    .get_temp      = &_get_temp,
    .compensate_temp_drift = &_compensate_temp_drift
};
