#include <canopus/types.h>
#include <canopus/logging.h>

#include <canopus/subsystem/subsystem.h>

#include "compression.h"

#ifdef ZLIB

#include <zlib.h>

static struct {
    int (*compress)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
    int (*uncompress)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
    uLong (*compressBound)(uLong sourceLen);
    uLong (*zlibCompileFlags)(void);
} zfunc = { compress, uncompress, compressBound, zlibCompileFlags }; /* upgradable */

#endif /* ZLIB */

retval_t cmd_mem_comp_init_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    void *a0, *a1, *a2, *a3;

    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a0)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a1)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a2)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a3)) return RV_NOSPACE;

    FUTURE_HOOK_6(cmd_mem_comp_init, a0, a1, a2, a3, iframe, oframe);

    return RV_SUCCESS;
}

retval_t cmd_mem_uncompress_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    void *a0, *a1, *a2, *a3;

    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a0)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a1)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a2)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a3)) return RV_NOSPACE;

    FUTURE_HOOK_6(cmd_mem_decompress, a0, a1, a2, a3, iframe, oframe);

    return RV_SUCCESS;
}

retval_t cmd_mem_compress_future(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    void *a0, *a1, *a2, *a3;

    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a0)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a1)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a2)) return RV_NOSPACE;
    if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&a3)) return RV_NOSPACE;

    FUTURE_HOOK_6(cmd_mem_compress, a0, a1, a2, a3, iframe, oframe);

    return RV_SUCCESS;
}

#ifdef ZLIB

retval_t cmd_mem_compress_z(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    Bytef *dest, *source;
    uLongf destLen, sourceLen;
    int rv;

	if (!frame_hasEnoughSpace(iframe, sizeof(Bytef *)*2+sizeof(uLongf)*2)) {
		return RV_NOSPACE;
	}

	source = (Bytef *)frame_get_u32_nocheck(iframe);
	sourceLen = frame_get_u32_nocheck(iframe);
	dest = (Bytef *)frame_get_u32_nocheck(iframe);
	destLen = frame_get_u32_nocheck(iframe);

    rv = zfunc.compress(dest, &destLen, source, sourceLen);
    frame_put_u32(oframe, rv);
    frame_put_u32(oframe, destLen); /* size of the compressed buffer */
    if (Z_OK == rv) {
        frame_put_u32(oframe, zfunc.compressBound(destLen));
    }

    return RV_SUCCESS;
}

retval_t cmd_mem_uncompress_z(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    Bytef *dest, *source;
    uLongf destLen, sourceLen;
    int rv;

	if (!frame_hasEnoughSpace(iframe, sizeof(Bytef *)*2+sizeof(uLongf)*2)) {
		return RV_NOSPACE;
	}

	source = (Bytef *)frame_get_u32_nocheck(iframe);
	sourceLen = frame_get_u32_nocheck(iframe);
	dest = (Bytef *)frame_get_u32_nocheck(iframe);
	destLen = frame_get_u32_nocheck(iframe);

    rv = zfunc.uncompress(dest, &destLen, source, sourceLen);
    frame_put_u32(oframe, rv);
    frame_put_u32(oframe, destLen); /* size of the uncompressed buffer */

    return RV_SUCCESS;
}

#endif /* ZLIB */
