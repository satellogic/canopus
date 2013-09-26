#ifndef _CANOPUS_SUBSYSTEM_MEMORY_COMPRESSION_H
#define _CANOPUS_SUBSYSTEM_MEMORY_COMPRESSION_H

#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/subsystem/subsystem.h>

retval_t cmd_mem_comp_init_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_mem_uncompress_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_mem_compress_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
#ifdef ZLIB
retval_t cmd_mem_compress_z(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
retval_t cmd_mem_uncompress_z(const subsystem_t *self, frame_t * iframe, frame_t * oframe);
#endif /* ZLIB */

retval_t cmd_mem_compress_bcl_lz(const subsystem_t *self, frame_t *iframe, frame_t *oframe);
retval_t cmd_mem_decompress_bcl_lz(const subsystem_t *self, frame_t *iframe, frame_t *oframe);

#endif
