/*
 * Low-level flash driver for AT49BV320DT
 *
 *  Created on: Jul 30, 2012
 *      Author: gera
 */

#ifndef AT49B320D_H_
#define AT49B320D_H_

#include <stdint.h>

typedef uint16_t flash_data_t;

#define FLASH_64k_COUNT       63
#define FLASH_64k_BLOCK_SIZE  (64*1024)
#define FLASH_64k_BASE	      0
#define FLASH_64k_TOP	      (FLASH_64k_BASE + FLASH_64k_BLOCK_SIZE * FLASH_64k_COUNT)
#define FLASH_8k_COUNT        8
#define FLASH_8k_BLOCK_SIZE   (8*1024)
#define FLASH_8k_BASE         FLASH_64k_TOP /* 0x3F 0000 */
#define FLASH_8k_TOP          (FLASH_8k_BASE + FLASH_8k_BLOCK_SIZE * FLASH_8k_COUNT)
#define FLASH_SIZE            FLASH_8k_TOP

#define IS_64k_SECTOR(address)	(((((uintptr_t)address) % FLASH_SIZE) >= FLASH_64k_BASE) && ((((uintptr_t)address) % FLASH_SIZE) < FLASH_64k_TOP))
#define IS_8k_SECTOR(address)	(((((uintptr_t)address) % FLASH_SIZE) >= FLASH_8k_BASE) && ((((uintptr_t)address) % FLASH_SIZE) < FLASH_8k_TOP))

#define FLASH_CMD_READ                     0xFF
#define FLASH_CMD_SECTOR_ERASE             0x20
#define FLASH_CMD_SECTOR_ERASE_CONFIRM     0xD0
#define FLASH_CMD_WORD_PROGRAM             0x40
#define _FLASH_CMD_DUAL_WORD_PROGRAM       0xE0
#define FLASH_CMD_SUSPEND_ACTION           0xB0
#define FLASH_CMD_RESUME_ACTION            0xD0
#define FLASH_CMD_PRODUCT_ID               0x90
#define FLASH_CMD_WRITE_LOCK_REGISTER      0x60
#define FLASH_CMD_SOFTLOCK                 0x01
#define FLASH_CMD_HARDLOCK                 0x2F
#define FLASH_CMD_UNLOCK                   0xD0
#define FLASH_CMD_READ_STATUS              0x70
#define FLASH_CMD_CLEAR_STATUS             0x50
#define FLASH_CMD_PROTECTION_REGISTER      0xC0
#define FLASH_CMD_LOCK_PROTECTION_OFFSET   0x80
#define FLASH_CMD_LOCK_PROTECTION_REGISTER 0xFFFD
#define FLASH_CMD_READ_PROTECTION_STATUS   0x90
#define FLASH_CMD_CFI_QUERY                0x98

/* Check Write State Machine (WSM) bit first to determine Word Program
 * or Sector Erase completion, before checking program or erase
 * status bits. */
#define FLASH_STATUS_WSM_READY		   (1<<7)

/* When FLASH_CMD_SUSPEND_ACTION is issued, WSM halts execution and sets
 * both FLASH_STATUS_WSMS_READY and FLASH_STATUS_ERASE_SUSPENDED bits to “1”
 * FLASH_STATUS_ERASE_SUSPENDED bit remains set to “1” until
 * an Erase Resume command is issued. */
#define FLASH_STATUS_ERASE_SUSPENDED   (1<<6)

/* When FLASH_STATUS_ERASE_ERROR is set, WSM has applied the max number of
 * erase pulses to the sector and is still unable to verify successful
 * sector erasure. */
#define FLASH_STATUS_ERASE_ERROR       (1<<5)

/* When FLASH_STATUS_PROGRAM_ERROR set, WSM has attempted but failed to
 * program a word */
#define FLASH_STATUS_PROGRAM_ERROR     (1<<4)

/* The VPP status bit does not provide continuous indication of VPP
 * level. The WSM interrogates VPP level only after the Program or
 * Erase command sequences have been entered and informs the
 * system if VPP has not been switched on. The VPP is also checked
 * before the operation is verified by the WSM. */
#define FLASH_STATUS_VPP_LOW           (1<<3)

/* When FLASH_CMD_SUSPEND_ACTION is issued, WSM halts execution and
 * sets both FLASH_STATUS_WSM_READY and FLASH_STATUS_PROGRAM_SUSPENDED.
 * FLASH_STATUS_PROGRAM_SUSPENDED bit remains set until a
 * FLASH_CMD_RESUME_ACTION command is issued. */
#define FLASH_STATUS_PROGRAM_SUSPENDED (1<<2)

/* If a Program or Erase operation is attempted to one of the locked
 * sectors, this bit is set by the WSM. The operation specified is
 * aborted and the device is returned to read status mode. */
#define FLASH_STATUS_SECTOR_LOCKED     (1<<1)
#endif /* AT49B320D_H_ */
