/*----------------------------------------------------------------------------*/
/* sys_link.cmd                                                               */
/*                                                                            */

#define FIRMWARE FLASH0A

/*----------------------------------------------------------------------------*/
/* Linker Settings                                                            */

/*----------------------------------------------------------------------------*/
/* Memory Map                                                                 */

MEMORY
{
    VECTORS (X)  : origin=0x00000000 length=0x00000020
    STAGE0 (RX)  : origin=0x00000020 length=0x00007fe0
    /* sectors 1, 2, 3 reserved */
    FLASH0A (RX) : origin=0x00020000 length=0x00060000
    FLASH0B (RX) : origin=0x00080000 length=0x00080000
    FLASH0C (RX) : origin=0x00100000 length=0x00080000

    FLASH1A (RX) : origin=0x00180000 length=0x00080000
    FLASH1B (RX) : origin=0x00200000 length=0x00080000
    FLASH1C (RX) : origin=0x00280000 length=0x00080000

    NVRAM   (R) : origin=0xf0200000 length=0x00008000
    FHOOKS  (R) : origin=0xf0208000 length=0x00004000
    /* not used */
    FLASH7  (R) : origin=0xf020c000 length=0x00004000

    STACKS  (RW) : origin=0x08000000 length=0x00001500
    RAM     (RW) : origin=0x08001500 length=0x0002eb00
    PD3     (RW) : origin=0x08030000 length=0x00010000
}

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */

SECTIONS
{
    .stage0.intvecs: > VECTORS

    GROUP {
        .stage0.text:
        .stage0.data:
    } > STAGE0

    GROUP {
        .app.intvecs: /* must come first */
        .text:
        FEE_TEXT_SECTION:
        .const:
        FEE_CONST_SECTION: palign(8)
        .pinit:
    } > FIRMWARE
    .cinit: > FIRMWARE /* must be out of group for correct .data compression */

    .fhooks: type = NOLOAD, run = FHOOKS > FHOOKS

    GROUP {
/*      .sysmem: */
        FEE_DATA_SECTION:
        .data:
        .bss:
        .heap:
    } > RAM
    .record: > PD3

}

/*----------------------------------------------------------------------------*/

