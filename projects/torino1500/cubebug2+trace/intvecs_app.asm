;-------------------------------------------------------------------------------
; sys_intvecs.asm
;
; (c) Texas Instruments 2009-2010, All rights reserved.
;

  .cdecls C,LIST,"stage0_cfg.h"
  .if STAGE0 == 0

        .sect ".app.intvecs"

;-------------------------------------------------------------------------------
; import reference for interrupt routines

        .ref _c_int00
        .ref vPortYieldProcessor
    .ref _dabort
    .def resetEntry

;-------------------------------------------------------------------------------
; interrupt vectors

resetEntry
        b   _c_int00                ; reset
undefEntry
        b   undefEntry				; undefined instruction
SWI
        b   vPortYieldProcessor		; software interrupt
prefetchEntry
        b   prefetchEntry			; Abort (prefetch)
ABORT
        b   _dabort					; Abort (data)
MAGIC
        .word STAGE0_MAGIC_UPGRADE ; Reserved
IRQ
        ldr pc,[pc,#-0x1b0]			; IRQ
FIQ
        ldr pc,[pc,#-0x1b0]			; FIQ

;-------------------------------------------------------------------------------

  .endif
