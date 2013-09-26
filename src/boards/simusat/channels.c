#ifdef __CHANNELS_C__
#error channels.c can only be included from init.c and only once
#else
#define __CHANNELS_C__

#include <canopus/drivers/channel.h>
#include <canopus/drivers/simusat/channel_posix.h>
#include <canopus/drivers/memory/channel_link_driver.h>

const channel_t memory_channel_0 = DECLARE_CHANNEL_MEMORY_SIZE(1024);
const channel_t memory_channel_1 = DECLARE_CHANNEL_MEMORY_SIZE(1024);

#endif /* __CHANNELS_C__ */
