#include <canopus/types.h>
#include <canopus/logging.h>
#include <canopus/md5.h>

#include <canopus/subsystem/subsystem.h>

#include <string.h>

#include "compression.h"

#include "comp_bcl/lz.h"

retval_t
cmd_mem_compress_bcl_lz(const subsystem_t *self, frame_t *iframe, frame_t *oframe)
{
	unsigned char *in, *out;
	unsigned int insize;
	int outsize;
	uint8_t digest_expected;

	if (!frame_hasEnoughSpace(iframe, sizeof(uint32_t)*3 + sizeof(uint8_t))) {
		return RV_NOSPACE;
	}

	in = (unsigned char *)frame_get_u32_nocheck(iframe);
	insize = frame_get_u32_nocheck(iframe);
	out = (unsigned char *)frame_get_u32_nocheck(iframe);
	digest_expected = frame_get_u8_nocheck(iframe);

	outsize = LZ_Compress(in, out, insize);
	frame_put_u32(oframe, outsize);

	if (0 == outsize) {
		return RV_ERROR;
	}
	if (digest_expected) {
		MD5_CTX ctx;

		memset(ctx.digest, 0, sizeof(ctx.digest));
		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *)out, outsize);
		MD5Final(&ctx);
		(void)frame_put_data(oframe, ctx.digest, sizeof(ctx.digest));
	}

	return RV_SUCCESS;
}

retval_t
cmd_mem_decompress_bcl_lz(const subsystem_t *self, frame_t *iframe, frame_t *oframe)
{
	unsigned char *in, *out;
	unsigned int insize;
	uint32_t expected_outsize;

	if (!frame_hasEnoughSpace(iframe, sizeof(uint32_t)*4)) {
		return RV_NOSPACE;
	}

	in = (unsigned char *)frame_get_u32_nocheck(iframe);
	insize = frame_get_u32_nocheck(iframe);
	out = (unsigned char *)frame_get_u32_nocheck(iframe);
	expected_outsize = frame_get_u32_nocheck(iframe);

	LZ_Uncompress(in, out, insize);
	if (expected_outsize) {
		MD5_CTX ctx;

		memset(ctx.digest, 0, sizeof(ctx.digest));
		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *)out, expected_outsize);
		MD5Final(&ctx);
		(void)frame_put_data(oframe, ctx.digest, sizeof(ctx.digest));
	}

	return RV_SUCCESS;
}
