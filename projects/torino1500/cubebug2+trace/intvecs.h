#ifndef _INTVECS_H
#define _INTVECS_H

#include <stdint.h>

enum isr_idx_e {
	RESET_INDEX = 0,
	UNDEF_INDEX = 1,
	SWI_INDEX = 2,
	PREFETCH_INDEX = 3,
	DATAABORT_INDEX = 4,
	RESERVED_INDEX = 5,
	IRQ_INDEX = 6,
	FIQ_INDEX = 7
};

typedef uint32_t arm_instr_t;

typedef /* __attribute__((noreturn)) */ void isr_t(void);

typedef struct arm_intvecs {
	arm_instr_t reset;
	arm_instr_t undef;
	arm_instr_t swi;
	arm_instr_t prefetch;
	arm_instr_t abort;
	arm_instr_t reserved;
	arm_instr_t irq;
	arm_instr_t fiq;
} intvecs_t;

typedef union stage0_hdr {
	struct {
		isr_t *reset;
		isr_t *undef;
		isr_t *swi;
		isr_t *prefetch;
		isr_t *abort;
		uint32_t magic;
		isr_t *irq;
		isr_t *fiq;
	} byName;
	isr_t *byIndex[8];
} stage0_hdr_t;

#define VALID_RESERVED_INSTR 0xeafffffe

#endif
