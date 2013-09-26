#ifndef __NANOWHEEL_H__
#define __NANOWHEEL_H__

#include <canopus/types.h>

typedef uint16_t nwheel_addr_t; /* FIXME u24 gera */

typedef enum nwheel_direction_e {
	clockwise = 0,
	counter_clockwise
} nwheel_direction_e;

#define NANOWHEEL_FDIR_OFF								0x0000
#define NANOWHEEL_FDIR_SOFT								0x0001
#define NANOWHEEL_FDIR_HARD								0x0002

#define NANOWHEEL_FAULT_MOSFET_SHORT_GND				0x0004
#define NANOWHEEL_FAULT_MOSFET_SHORT_VDD				0x0008
#define NANOWHEEL_FAULT_VREG_UNDERVOLTAGE				0x0010
#define NANOWHEEL_FAULT_VBOOT_UNDERVOLTAGE				0x0020
#define NANOWHEEL_FAULT_A4935_OVERTEMP					0x0040
#define NANOWHEEL_FAULT_VDD_UNDERVOLTAGE				0x0080

#define NANOWHEEL_FAULT_VDD_OVERVOLTAGE_12				0x0000
#define NANOWHEEL_FAULT_VDD_OVERVOLTAGE_14				0x0100
#define NANOWHEEL_FAULT_VDD_OVERVOLTAGE_16				0x0200
#define NANOWHEEL_FAULT_VDD_OVERVOLTAGE_18				0x0300

#define NANOWHEEL_FAULT_MOTOR_OVERCURRENT_0_5			0x0000
#define NANOWHEEL_FAULT_MOTOR_OVERCURRENT_1				0x0400
#define NANOWHEEL_FAULT_MOTOR_OVERCURRENT_1_5			0x0800
#define NANOWHEEL_FAULT_MOTOR_OVERCURRENT_2				0x0C00

typedef enum nanowheel_tlm_e {
    NANOWHEEL_TLM_VOLTAGE       = 0,
    NANOWHEEL_TLM_CURRENT       = 1,
    NANOWHEEL_TLM_TEMP1         = 2,
    NANOWHEEL_TLM_TEMP2         = 3,
    NANOWHEEL_TLM_TEMP3         = 4,
    NANOWHEEL_TLM_TEMP4         = 5,
    NANOWHEEL_TLM_SPEED         = 6,
    NANOWHEEL_TLM_STATUS        = 7,
    NANOWHEEL_TLM_CMD_VOLTAGE   = 8,
    NANOWHEEL_TLM_FAULT_FLAGS   = 9,
    NANOWHEEL_TLM_MAX
} nanowheel_tlm_e;

#define NANOWHEEL_TLM_VOLTAGE_MASK     					(1 << NANOWHEEL_TLM_VOLTAGE)
#define NANOWHEEL_TLM_CURRENT_MASK     					(1 << NANOWHEEL_TLM_CURRENT)
#define NANOWHEEL_TLM_TEMP1_MASK       					(1 << NANOWHEEL_TLM_TEMP1)
#define NANOWHEEL_TLM_TEMP2_MASK      					(1 << NANOWHEEL_TLM_TEMP2)
#define NANOWHEEL_TLM_TEMP3_MASK       					(1 << NANOWHEEL_TLM_TEMP3)
#define NANOWHEEL_TLM_TEMP4_MASK       					(1 << NANOWHEEL_TLM_TEMP4)
#define NANOWHEEL_TLM_SPEED_MASK       					(1 << NANOWHEEL_TLM_SPEED)
#define NANOWHEEL_TLM_STATUS_MASK      					(1 << NANOWHEEL_TLM_STATUS)
#define NANOWHEEL_TLM_CMD_VOLTAGE_MASK 					(1 << NANOWHEEL_TLM_CMD_VOLTAGE)
#define NANOWHEEL_TLM_FAULT_FLAGS_MASK					(1 << NANOWHEEL_TLM_FAULT_FLAGS)
#define NANOWHEEL_TLM_ALL_MASK	(0 \
	| NANOWHEEL_TLM_VOLTAGE_MASK		\
	| NANOWHEEL_TLM_CURRENT_MASK     	\
	| NANOWHEEL_TLM_TEMP1_MASK       	\
	| NANOWHEEL_TLM_TEMP2_MASK      	\
	| NANOWHEEL_TLM_TEMP3_MASK       	\
	| NANOWHEEL_TLM_TEMP4_MASK       	\
	| NANOWHEEL_TLM_SPEED_MASK       	\
	| NANOWHEEL_TLM_STATUS_MASK      	\
	| NANOWHEEL_TLM_CMD_VOLTAGE_MASK 	\
	| NANOWHEEL_TLM_FAULT_FLAGS_MASK )

#define NANOWHEEL_FLASH_BLOCKSIZE 512

#define ADC(x)						((x*2.5)/4095)
#define TLM_FIRMWARE_VERSION(x)		((x & 0x7E00)>>9)
#define TLM_TEMPERATURE(x)			((ADC(x)-0.424)/0.00625)
#define TLM_VOLTAGE(x)				((ADC(x)*(11.9/1.175)))

typedef struct nwheel_tlm {
    uint16_t values[NANOWHEEL_TLM_MAX];
} nwheel_tlm_t;

/* Commands */

retval_t nwheel_ping(void);
retval_t nwheel_reset(void);

retval_t nwheel_coast(void);
retval_t nwheel_brake(void);

retval_t nwheel_set_voltage(const uint16_t voltage);
retval_t nwheel_set_speed(const uint16_t speed);
retval_t nwheel_set_direction(const nwheel_direction_e direction);

retval_t nwheel_FDIR_set(const uint16_t config);
retval_t nwheel_FDIR_reset(void);

retval_t nwheel_TLM_get(const uint16_t mask, nwheel_tlm_t *tlm);
/* retval_t nwheel_TLM_log_start();    */
/* retval_t nwheel_TLM_log_downlaod(); */

retval_t nwheel_memory_read(nwheel_addr_t nwheel_addr, void *obc_ptr, uint16_t size);
retval_t nwheel_memory_write(nwheel_addr_t nwheel_addr, const void *obc_ptr, uint16_t size);
retval_t nwheel_memory_call(const uint32_t address, uint16_t param1, uint16_t param2, uint8_t *reply);
retval_t nwheel_flash_write(nwheel_addr_t nwheel_flash_addr /* to */, nwheel_addr_t nwheel_addr /* from */, uint16_t size);

retval_t nwheel_memory_md5(nwheel_addr_t nwheel_addr, uint16_t size, void *md5_dstptr);
retval_t nwheel_flash_write_1block_aligned(nwheel_addr_t nwheel_flash_addr /* to */, void *obc_bufptr /* from */, nwheel_addr_t size, uint16_t nwheel_ram_addr /* tmpbuf */);
retval_t nwheel_flash_write_blocks_aligned(nwheel_addr_t nwheel_flash_addr /* to */, void *obc_bufptr /* from */, nwheel_addr_t size, uint16_t nwheel_ram_addr /* tmpbuf */);
retval_t nwheel_jump_to_low_version();
retval_t nwheel_jump_to_high_version(); /* black magic! */
#endif /* __NANOWHEEL_H__ */
