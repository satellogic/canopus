#include <canopus/types.h>
#include <canopus/drivers/console_lowlevel.h>

#include <string.h>

#include <FreeRTOS.h>
#include <semphr.h>

#ifdef DEBUG_CONSOLE_ENABLED

static xSemaphoreHandle xMutex = NULL;
static const portTickType xBlockTime = 5 / portTICK_RATE_MS;

void
lowlevel_console_rtos_enable(bool state)
{
    if (true == state) {
        xMutex = xSemaphoreCreateRecursiveMutex();
    } else if (NULL != xMutex) {
		#ifndef portUSING_MPU_WRAPPERS /* FIXME broken FreeRTOS_MPU port on TMS570... */
        vSemaphoreDelete(xMutex);
		#endif
    }
}

static bool
console_lock()
{
    if (NULL == xMutex) {
        return true;
    }
    if (pdFALSE == xSemaphoreTakeRecursive(xMutex, xBlockTime)) {
        return false;
    }
    return true;
}

static void
console_unlock()
{
    if (NULL == xMutex) {
        return;
    }
    xSemaphoreGiveRecursive(xMutex);
}

#define LOCK() \
    if (true != console_lock()) { \
        return; \
    }
#define UNLOCK() \
    console_unlock();

void
lowlevel_console_putbuf(const void *buf, size_t count)
{
    uint8_t *c = (uint8_t *)buf;

    if (NULL == buf || 0 == count) {
        return;
    }
    LOCK();
    while (count--) {
        lowlevel_console_putchar(*c++);
    }
    UNLOCK();
}

void
lowlevel_console_putnewline()
{
    LOCK();
    lowlevel_console_putchar('\r');
    lowlevel_console_putchar('\n');
    UNLOCK();
}

void
lowlevel_console_putstring(const char *string)
{
    if (string == NULL) {
        return;
    }
    LOCK();
    while (*string != '\0') {
        lowlevel_console_putchar(*string++);
    }
    UNLOCK();
}

static char ascii(uint8_t c) {
    if (c <= 9) {
        return c + '0';
    } else if (c >= 0xa && c <= 0xf) {
        return c + 'a' - 10;
    } else {
        return '?';
    }
}

void
lowlevel_console_puthex8(uint8_t val8)
{
    LOCK();
    lowlevel_console_putchar(ascii(val8 >> 4));
    lowlevel_console_putchar(ascii(val8 & 0xf));
    UNLOCK();
}

void
lowlevel_console_puthex32(uint32_t val32)
{
    register int i;

    LOCK();
    for (i = 0; i < 4; i++) {
        lowlevel_console_puthex8(val32 >> ((3 - i) * 8));
    }
    UNLOCK();
}

void
lowlevel_console_putstrhex32(const char *string, uint32_t val32)
{
    LOCK();
    lowlevel_console_putstring(string);
    lowlevel_console_puthex32(val32);
    lowlevel_console_putnewline();
    UNLOCK();
}

/*
uint32_t
gethex32(const char *p)
{
	int i;
	long val = 0;

	for (i = 0; i < 8; i++, p++) {
		val <<= 4;
		if (*p >= '0' && *p <= '9') {
			val |= *p - '0';
		} else if (*p >= 'a' && *p <= 'f') {
			val |= *p - 'a' + 10;
		} else if (*p >= 'A' && *p <= 'F') {
			val |= *p - 'A' + 10;
		}
	}

	return val;
}
*/

void lowlevel_console_hexdump_ex(const void *ptr, size_t size, const char *desc, hexdump_flag_e flags)
{
    register int i;
    register uint8_t *buf = (uint8_t *)ptr;

    LOCK();
    if (desc != NULL) {
        lowlevel_console_putstring(desc);
        lowlevel_console_putchar(' ');
    }
    if (flags & HEXDUMP_ADDRESS) {
        lowlevel_console_putstring("at 0x");
        lowlevel_console_puthex32((uintptr_t)ptr);
        lowlevel_console_putchar(':');
    }
    if (desc != NULL || (flags & HEXDUMP_ADDRESS)) {
        lowlevel_console_putnewline();
    }
    for (i = 0; i < size; i++) {
        lowlevel_console_puthex8(buf[i]);
        if (flags & HEXDUMP_GROUP4_32) {
            if (!i) {
                continue;
            }
            if (!((i + 1) % 4)) {
                lowlevel_console_putchar(' ');
            }
            if (!((i + 1) % 32)) {
                lowlevel_console_putnewline();
            }
        }
    }
    lowlevel_console_putnewline();
    UNLOCK();
}

void
lowlevel_console_hexdump(const void *ptr, size_t size, const char *desc)
{
    lowlevel_console_hexdump_ex(ptr, size, desc, HEXDUMP_BASIC);
}

#endif
