;-------------------------------------------------------------------------------
; sys_intvecs.asm
;
; (c) Texas Instruments 2009-2010, All rights reserved.
;

  .cdecls C,LIST,"stage0_cfg.h"

  .if STAGE0 == 1

        .sect ".stage0.intvecs"
        .retain

;-------------------------------------------------------------------------------
; interrupt vectors

resetEntry
        b   RESET_trampoline    ; reset
undefEntry
        b   UNDEF_trampoline    ; Undefined instruction
SWI
        b   SWI_trampoline
prefetchEntry
        b   PREFETCH_trampoline ; Abort (prefetch)
ABORT
        b   DATAABORT_trampoline; Abort (data)
MAGIC
        .word STAGE0_MAGIC_BOOTROM; Reserved
IRQ
        ldr pc,[pc,#-0x1b0]	; IRQ
FIQ
        ldr pc,[pc,#-0x1b0]	; FIQ


        .sect ".stage0.text"

RESET_trampoline:
        .ref    get_RESET_dispatcher
        BL      get_RESET_dispatcher
        BX      R0

DEFINE_TRAMPOLINE             .macro          dispatcher_getter
        .ref dispatcher_getter
        SUB             SP, SP, #4          ; Space for handler
        STMFD           SP!, {R0}           ; Save R0
        ADD             R0, SP, #4          ; Load argument. Pointer to handler
        STMFD           SP!, {R1-R12,LR}    ; Save all regs
        BL              dispatcher_getter   ; Get handler from options
        LDMFD           SP!, {R1-R12,LR}    ; Restore all regs
        LDMFD           SP!, {R0}           ; Restore R0
        LDMFD           SP!, {PC}           ; Jump to handler
 .endm

UNDEF_trampoline:
    DEFINE_TRAMPOLINE   get_UNDEF_dispatcher
SWI_trampoline:
    DEFINE_TRAMPOLINE   get_SWI_dispatcher
PREFETCH_trampoline:
    DEFINE_TRAMPOLINE   get_PREFETCH_dispatcher
DATAABORT_trampoline:
    DEFINE_TRAMPOLINE   get_DATAABORT_dispatcher

  .endif

;-------------------------------------------------------------------------------
