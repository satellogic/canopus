#include <canopus/types.h>

#include "intvecs.h"
#include "stage0_cfg.h"

#if (STAGE0 == 1)

#pragma SET_DATA_SECTION (".stage0.data")

const stage0_hdr_t *const sectors_addr[] = {

		/* SWI: first try SRAM */

		(stage0_hdr_t *)0x08000000, /* SRAM noremap */

		/* all: try alterning banks */

		(stage0_hdr_t *)0x00180000, /* Bank#1,Sectors#0-3 */
		(stage0_hdr_t *)0x00080000, /* Bank#0,Sectors#7-10 */

		(stage0_hdr_t *)0x00200000, /* Bank#1,Sectors#4-7 */
		(stage0_hdr_t *)0x00100000,	/* Bank#0,Sectors#11-14 */

		(stage0_hdr_t *)0x00280000, /* Bank#1,Sectors#8-11 */

		(stage0_hdr_t *)0x00000000, /* Bank#0,Sectors#0-6 (at the end, last one tried) */
};

#pragma SET_CODE_SECTION (".stage0.text")

/* this function can not use the .stack */
extern isr_t *_c_int00;
isr_t **get_RESET_dispatcher(void)
{
	register stage0_hdr_t **hdr = (stage0_hdr_t **)&sectors_addr[1]; /* skip SRAM (first entry) */
	do {
		if (STAGE0_MAGIC_FIRMWARE == (*hdr)->byName.magic) {
			return &((*hdr)->byIndex[RESET_INDEX]);
		}
	} while (*(++hdr)); /* loop while not self */
	return &_c_int00;
}

/* once called by DEFINE_TRAMPOLINE those functions can use the .stack */
#define ISR_DISPATCH(NAME, IDX, FUNC) \
extern isr_t *FUNC; \
isr_t **NAME(isr_t ***fptr) \
{ \
	register stage0_hdr_t **hdr = (stage0_hdr_t **)sectors_addr; \
	do { \
		if (STAGE0_MAGIC_FIRMWARE == (*hdr)->byName.magic) { \
			if (fptr != NULL) { \
				*fptr = &((*hdr)->byIndex[IDX]); \
			} \
			return &((*hdr)->byIndex[IDX]); \
		} \
	} while (*(++hdr)); /* loop while not self */ \
	if (fptr != NULL) { \
		*fptr = &FUNC; \
	} \
	return &FUNC; \
}

ISR_DISPATCH(get_UNDEF_dispatcher, UNDEF_INDEX, _c_int00);
ISR_DISPATCH(get_SWI_dispatcher, SWI_INDEX, vPortYieldProcessor);
ISR_DISPATCH(get_PREFETCH_dispatcher, PREFETCH_INDEX, _c_int00); // XXX dunno if we can use stack here
ISR_DISPATCH(get_DATAABORT_dispatcher, DATAABORT_INDEX, _dabort); // TODO maybe use custom_dabort()

#if 0

/* how it is used in sys_intvecs.asm */

void RESET_dispatch()
{
	isr_t *f;

	f = get_dispatcher(NULL, RESET_INDEX);

	(*f)();
}

void SWI_dispatch()
{
	isr_t *f;

	get_dispatcher(&f, SWI_INDEX);

	(*f)();
}
#endif

#endif /* (STAGE0 == 1) */
