#ifndef _CANOPUS_SUBSYSTEM_MEMORY_STRING_H
#define _CANOPUS_SUBSYSTEM_MEMORY_STRING_H

#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/subsystem/subsystem.h>

retval_t cmd_mem_bitmap(const subsystem_t *self, frame_t *iframe, frame_t *oframe);

#endif
