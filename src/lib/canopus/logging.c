#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/logging.h>
#include <canopus/stringformat.h>

#ifdef VARARGS_SUPPORTED
#include <stdarg.h>
#endif

#include <stdint.h>
#include <string.h>
#include <ctype.h> // isprint()

#include <FreeRTOS.h>
#include <task.h>

#define LOGGERS_COUNT 4

#define DEBUG_LOG

#define CONSOLE_LINESZ_MAX 128

static log_report_nb_t *loggers[LOGGERS_COUNT] = {
    ARRAY_INITIALIZE_DEFAULT_VALUE(LOGGERS_COUNT, log_report_nb_t *),
};

static const portTickType log_timeout = portMAX_DELAY; /* FIXME */

/* ====================== log masks ====================== */

static logmask_t current_mask = LOGMASK_MAX;

void
dumpmask(logmask_t mask)
{
    #ifdef DEBUG_LOG
    #define _func(a) [ LOG_##a ] = #a
    #define _sep ,
    static const char *const logname[] = {
        LOGNAME_LIST()
    };
    #undef _func
    #undef _sep
    int i;

    for (i = 0; i < ARRAY_COUNT(logname); i++) {
        if (!(bitmask(i) & mask)) continue;
        if (NULL != logname[i]) {
            log_report_fmt(LOG_ALL, "log enabled 0x%010llx [%s]\r\n", bitmask(i), logname[i]);
        } else {
            log_report_fmt(LOG_ALL, "log enabled 0x%010llx (%d)\r\n", bitmask(i), i);
        }
    }
    #endif /* DEBUG_LOG */
}

#undef bitmask /* XXX remove me */
logmask_t
bitmask(logbit_t bit)
{
    return UINT64_C(1) << bit;
}

bool
log_enabled(logbit_t bit)
{
    if (LOG_ALL == bit) return true;
    return bitmask(bit) & current_mask ? true : false;
}

logmask_t
log_setmask(logmask_t new_mask, bool dump)
{
    logmask_t previous_mask = current_mask;

    current_mask = new_mask;
    if (dump) {
        dumpmask(new_mask);
    }

    return previous_mask;
}

/* ====================== generig logging ====================== */

static bool
loggers_registered()
{
    int i;

    for (i = 0; i < ARRAY_COUNT(loggers); i++) {
        if (IS_PTR_VALID(loggers[i])) {
            return true;
        }
    }

    return false;
}

void
log_register(int index, log_report_nb_t *logger)
{
    if (index < 0 || index >= ARRAY_COUNT(loggers)) return;

    loggers[index] = logger;
}

void
log_report_nb(portTickType xBlockTime, logbit_t bit, const char *msg)
{
    int i;

    FUTURE_HOOK_2(log_report_nb, bit, msg);

    for (i = 0; i < ARRAY_COUNT(loggers); i++) {
        if (IS_PTR_VALID(loggers[i])) {
            loggers[i](xBlockTime, bit, msg);
        }
    }
}

void
log_report(logbit_t bit, const char *msg)
{
    log_report_nb(log_timeout, bit, msg);
}

#ifdef VARARGS_SUPPORTED /* va_list is C89 */

static void
log_report_varargs_nb(portTickType xBlockTime, logbit_t bit, const char *fmt, const va_list *argp)
{
    char buf[CONSOLE_LINESZ_MAX];
    int sz;

    if (NULL == fmt || !log_enabled(bit) || !loggers_registered()) {
        /* save time. */
        return;
    }
    sz = vsnprintf(buf, sizeof(buf), fmt, *argp);
    if (sz <= 0) {
        /* nothing to print */
        return;
    }
    if (sz >= sizeof(buf)) {
        /* shorten the output */
        buf[sizeof(buf)-1] = '\n';
    }

    log_report_nb(xBlockTime, bit, buf);
}

void
log_report_varargs(logbit_t bit, const char *fmt, const va_list *argp)
{
    log_report_varargs_nb(log_timeout, bit, fmt, argp);
}

void
log_report_fmt_nb(portTickType xBlockTime, logbit_t bit, const char *fmt, ...)
{
    va_list argp;

    if (NULL == fmt || !log_enabled(bit) || !loggers_registered()) {
        /* save time. */
        return;
    }

    va_start(argp, fmt);
    log_report_varargs_nb(xBlockTime, bit, fmt, &argp);
    va_end(argp);
}

void
log_report_fmt(logbit_t bit, const char *fmt, ...)
{
    va_list argp;

    if (NULL == fmt || !log_enabled(bit) || !loggers_registered()) {
        /* save time. */
        return;
    }

    va_start(argp, fmt);
    log_report_varargs_nb(log_timeout, bit, fmt, &argp);
    va_end(argp);
}
#endif /* VARARGS_SUPPORTED */

#define XXD_BUFSIZE 16

static void
log_report_xxd16(logbit_t bit, uint8_t *iptr, size_t size)
{
    uint8_t ibuf[XXD_BUFSIZE];
    char obuf[10 + XXD_BUFSIZE*3 + 2 + 16 + 2], tbuf[4];
    uintptr_t addr = (uintptr_t)iptr;
    int i;
    static uint8_t blank[] = "   ";

    memcpy(ibuf, iptr, size);
    memset(ibuf + size, 0, sizeof(ibuf) - size);
    memset(obuf, ' ', sizeof(obuf));

    sprintf(obuf, "%08lx: ", addr);
    for (i = 0; i < sizeof(ibuf); i++) {
        if (i < size) {
            sprintf(tbuf, "%02x ", ibuf[i]);
            memcpy(obuf + 10 + i*3, tbuf, 3);
            obuf[10 + XXD_BUFSIZE*3 + 2 + i] = isprint(ibuf[i]) ? ibuf[i] : '.';
        } else {
            memcpy(obuf + 10 + i*3, blank, 3);
        }
    }
    obuf[10 + XXD_BUFSIZE*3 + 2 + 16 + 0] = '\n';
    obuf[10 + XXD_BUFSIZE*3 + 2 + 16 + 1] = '\0';

    log_report_nb(log_timeout, bit, obuf);
}

void
log_report_xxd(logbit_t bit, const void *vptr, size_t size)
{
    uint8_t *ptr = (uint8_t *)vptr;

    if (0 == size || NULL == vptr || !log_enabled(bit) || !loggers_registered()) {
        /* save time. */
        return;
    }
    while (0 != size) {
        size_t csize = size <= XXD_BUFSIZE ? size : XXD_BUFSIZE;
        log_report_xxd16(bit, ptr, csize);
        ptr += csize;
        size -= csize;
    }
}

const char *
retval_s(retval_t rv)
{
    static const char *retval_n[] = {
        [RV_SUCCESS] = "SUCCESS!",
        [RV_TIMEOUT] = "TIMEOUT",
        [RV_BUSY] = "BUSY",
        [RV_ILLEGAL] = "ILLEGAL",
        [RV_NOSPACE] = "NOSPACE",
        [RV_ERROR] = "ERROR",
        [RV_EXIST] = "EXIST",
        [RV_NOENT] = "NOENT",
        [RV_PERM] = "PERM",
        [RV_PARTIAL] = "PARTIAL",
        [RV_NACK] = "NACK",
        [RV_NOTIMPLEMENTED] = "NOTIMPLEMENTED",
    };

    if (rv >= ARRAY_COUNT(retval_n) || NULL == retval_n[rv]) {
    	return "BUGGY";
    }

    return retval_n[rv];
}

/* ====================== log to channel ====================== */

#include <canopus/drivers/channel.h>

static channel_t *out_chan;

static frame_t fhdr;

static bool log_ticks = true;

static void
log_channel_report_nb(portTickType xBlockTime, logbit_t bit, const char *msg)
{
    frame_t fmsg;
    retval_t rv = RV_SUCCESS;

    FUTURE_HOOK_2(log_channel_report_nb, &bit, msg);

    if (NULL == out_chan || NULL == msg || !log_enabled(bit)) { // FIXME not needed
        return;
    }

    if (channel_lock(out_chan, xBlockTime) == RV_TIMEOUT)
        return;

    /* ticks */
    if (log_ticks) {
        static char buf[12];

        sprintf(buf, "[%08lx] ", xTaskGetTickCount());
        fmsg.buf = (uint8_t *)buf;
        fmsg.size = strlen(buf);
        frame_reset(&fmsg);
        channel_send(out_chan, &fmsg);
    }

    /* Prepend header, if any */
    if (fhdr.size > 0) {
        frame_reset(&fhdr);
        rv = channel_send(out_chan, &fhdr);
    }

    /* Print the actual message */
    if (RV_SUCCESS == rv) {
        fmsg.buf = (uint8_t *)msg;
        fmsg.size = strlen(msg);
        frame_reset(&fmsg);
        (void)channel_send(out_chan, &fmsg);
    }

    channel_unlock(out_chan);
}

void
log_channel_init(const channel_t *const out, const char *const prompt)
{
    if (NULL == out) return;

    log_register(0 /* default */, &log_channel_report_nb);

    out_chan = (channel_t *)out;
    if (NULL != prompt) {
        fhdr.buf = (uint8_t *)prompt;
        fhdr.size = strlen(prompt);
    }
}
