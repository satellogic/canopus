#ifdef USE_DEBUG_DUMP

#include <canopus/debug.h>
#include <canopus/drivers/console_lowlevel.h>

void
debug_dump(void *ptr, size_t size, const char *desc)
{
    return lowlevel_console_hexdump(ptr, size, desc);
}
#endif
