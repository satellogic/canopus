#ifndef _CANOPUS_LOGGING_H
#define _CANOPUS_LOGGING_H

#include <canopus/types.h>
#include <canopus/stringformat.h>
#ifdef VARARGS_SUPPORTED
#include <stdarg.h>
#endif
#include <FreeRTOS.h>
#include <task.h>

#define LOGNAME_LIST() \
    _func(GLOBAL) _sep                  /*  0 */ \
    _func(DEPRECATED) _sep \
    \
    /* subsystems */ \
    _func(SS_PLATFORM) _sep             /*  2 */ \
    _func(SS_POWER) _sep                /*  3 */ \
    _func(SS_MEMORY) _sep \
    _func(SS_MEMORY_VERBOSE) _sep \
    _func(SS_AOCS) _sep \
    _func(SS_AOCS_VERBOSE) _sep \
    _func(SS_THERMAL) _sep \
    _func(SS_CDH) _sep \
    _func(SS_CDH_ANTENNA) _sep \
    \
    _func(FRAME) _sep \
    _func(FRAME_VERBOSE) _sep \
    _func(CHANNELS) _sep \
    \
    /* devices */ \
    _func(WDG) _sep \
    _func(STB) _sep                     /* 15 */ \
    _func(STB_VERBOSE) _sep             /* 16 */ \
    _func(EPS) _sep                     /* 17 */ \
    _func(RADIO) _sep                   /* 18 */ \
    _func(RADIO_VERBOSE) _sep           /* 19 */ \
    _func(ANTENNA) _sep                 /* 20 */ \
    _func(I2C) _sep \
    _func(I2C_VERBOSE) _sep             /* 22 */ \
    _func(SPI) _sep \
    _func(MTQ) _sep \
    _func(MTQ_VERBOSE) _sep \
    _func(GYRO) _sep \
    _func(MAGNETOMETER) _sep \
    _func(WHEEL) _sep \
    _func(WHEEL_VERBOSE) _sep \
    _func(STARTRACKER) _sep             /* 30 */ \
    _func(TORINO) _sep \
    _func(OVERO) _sep \
    _func(HEATER) _sep \
    _func(THERMOMETER) _sep \
    _func(FLASH) _sep                   /* 32+3 */ \
    _func(NVRAM) _sep                   /* 32+4 */ \
    _func(SOCKET) _sep \
    \
    _func(I2C_FROM_ISR) _sep /* this DOES crash... */ \
    _func(INA209) _sep \
    \
    _func(ALL) _sep

#define _func(a) LOG_##a
#define _sep ,
typedef enum {
    LOGNAME_LIST()
} logbit_t;
#undef _func
#undef _sep

typedef uint64_t logmask_t;
#define LOGMASK_MAX UINT64_MAX

//logmask_t bitmask(logbit_t bit); /* XXX define me */
#define bitmask(bit) (UINT64_C(1) << bit) /* XXX rename me caps */

bool log_enabled(logbit_t bit);
logmask_t log_setmask(logmask_t mask, bool dump);
void dumpmask(logmask_t mask);
#define dumpmasks() dumpmask(LOGMASK_MAX)

typedef void log_report_nb_t(portTickType xBlockTime, logbit_t bit, const char *msg);
void log_register(int index, log_report_nb_t *logger);

void log_report(logbit_t bit, const char *msg);
#ifdef VARARGS_SUPPORTED
void log_report_fmt(logbit_t bit, const char *fmt, ...);
void log_report_varargs(logbit_t bit, const char *fmt, const va_list *argp);
#endif /* VARARGS_SUPPORTED */
void log_report_xxd(logbit_t bit, const void *ptr, size_t size);

const char *retval_s(retval_t rv);

#include <canopus/drivers/channel.h>

void log_channel_init(const channel_t *const out, const char *const prompt);

#define LOGMASK_DEFAULT_FLIGHT bitmask(LOG_SS_PLATFORM)|bitmask(LOG_SS_CDH_ANTENNA)

#define LOGMASK_DEFAULT_DEV ( \
            bitmask(LOG_GLOBAL)|bitmask(LOG_FRAME)|bitmask(LOG_SOCKET)| \
            bitmask(LOG_NVRAM)|bitmask(LOG_FLASH)| \
            bitmask(LOG_CHANNELS)|bitmask(LOG_SPI)| \
            bitmask(LOG_STB)|bitmask(LOG_STB_VERBOSE)| \
            bitmask(LOG_I2C_VERBOSE)|\
            bitmask(LOG_I2C)|bitmask(LOG_EPS)| \
            bitmask(LOG_MTQ)|bitmask(LOG_WHEEL)|bitmask(LOG_MAGNETOMETER)|bitmask(LOG_OVERO)| \
            bitmask(LOG_SS_PLATFORM)|bitmask(LOG_SS_POWER)| \
            bitmask(LOG_SS_MEMORY)| \
            bitmask(LOG_SS_THERMAL)|bitmask(LOG_SS_CDH)| \
            bitmask(LOG_SS_CDH_ANTENNA)| \
            bitmask(LOG_SS_AOCS)|bitmask(LOG_SS_AOCS_VERBOSE)|bitmask(LOG_MTQ_VERBOSE) \
        )


#define LOGMASK_DEFAULT LOGMASK_DEFAULT_FLIGHT

#define LOG_REPORT_MD5(log_bit, desc, md5_ctx_ptr) \
    log_report_fmt(log_bit, "%s%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", desc, \
        (md5_ctx_ptr)->digest[0],  (md5_ctx_ptr)->digest[1],  (md5_ctx_ptr)->digest[2],  (md5_ctx_ptr)->digest[3],  \
        (md5_ctx_ptr)->digest[4],  (md5_ctx_ptr)->digest[5],  (md5_ctx_ptr)->digest[6],  (md5_ctx_ptr)->digest[7],  \
        (md5_ctx_ptr)->digest[8],  (md5_ctx_ptr)->digest[9],  (md5_ctx_ptr)->digest[10], (md5_ctx_ptr)->digest[11], \
        (md5_ctx_ptr)->digest[12], (md5_ctx_ptr)->digest[13], (md5_ctx_ptr)->digest[14], (md5_ctx_ptr)->digest[15])

#endif
