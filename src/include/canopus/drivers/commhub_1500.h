#ifndef _COMMHUB_1500_H_
#define _COMMHUB_1500_H_

#include <canopus/types.h>

#define COMMHUB_REG_NC              	0x00
#define COMMHUB_REG_STATUS          	0x01
#define COMMHUB_REG_AMUX            	0x02
#define COMMHUB_REG_GP_OUT          	0x03
#define COMMHUB_REG_GP_OUT_HIZ_MASK    	0x04
#define COMMHUB_REG_GP_OUT_SHIFTER     	0x05
#define COMMHUB_REG_PSW             	0x06
#define COMMHUB_REG_UART_OVERO_RX   	0x07
#define COMMHUB_REG_UART_TMS_RX     	0x08
#define COMMHUB_REG_UART_OTHER0_RX  	0x09
#define COMMHUB_REG_UART_OTHER1_RX  	0x0A
#define COMMHUB_REG_POWER           	0x0B
#define COMMHUB_REG_GP_IN           	0x0C
#define COMMHUB_REG_COUNTER_0			0x0D
#define COMMHUB_REG_COUNTER_1			0x0E
#define COMMHUB_REG_COUNTER_2			0x0F
#define COMMHUB_REG_COUNTER_3			0x10
#define COMMHUB_REG_SYNC				0x11
#define COMMHUB_REG_VERSION				0x12

#define COMMHUB_UART_ADDR_LITHIUM 			0
#define COMMHUB_UART_ADDR_CONDOR 			1
#define COMMHUB_UART_ADDR_SVIP_0 			2
#define COMMHUB_UART_ADDR_SVIP_1 			3
#define COMMHUB_UART_ADDR_SPOT				4
#define COMMHUB_UART_ADDR_UMB 				5
#define COMMHUB_UART_ADDR_TMS_0_UART_0 		6
#define COMMHUB_UART_ADDR_TMS_0_UART_1 		7
#define COMMHUB_UART_ADDR_OVERO_UART_1		8
#define COMMHUB_UART_ADDR_HIGH				9
#define COMMHUB_UART_ADDR_LOW				10
#define COMMHUB_UART_ADDR_HIZ				15

#define COMMHUB_DEV_POWER_OVERO			(1 << 0)
#define COMMHUB_DEV_POWER_TMS_OTHER		(1 << 1)

#define COMMHUB_GP_IN_ANT_SW_1				(1 << 0)
#define COMMHUB_GP_IN_ANT_SW_2				(1 << 1)
#define COMMHUB_GP_IN_ANT_SW_3				(1 << 2)
#define COMMHUB_GP_IN_ANT_SW_4				(1 << 3)
#define COMMHUB_GP_IN_IMU_DRDY				(1 << 4)
#define COMMHUB_GP_IN_PD_3v3_WARN			(1 << 5)
#define COMMHUB_GP_IN_PD_3v3_OVER_LIMIT 	(1 << 6)
#define COMMHUB_GP_IN_PD_3v3_ALERT			(1 << 7)

#define COMMHUB_GP_OUT_LI1_NRST		(1 << 0)
#define COMMHUB_GP_OUT_IMU_PWR		(1 << 4)
#define COMMHUB_GP_OUT_OCT_AMUX		(1 << 8)
#define COMMHUB_GP_OUT_CONDOR_PWR	(1 << 10)
#define COMMHUB_GP_OUT_SVIP_PWR		(1 << 12)
#define COMMHUB_GP_OUT_SDR_PWR		(1 << 13)
#define COMMHUB_GP_OUT_GPS_PWR		(1 << 14)

#define COMMHUB_GP_OUT_SHR_DATA		(1 << 0)
#define COMMHUB_GP_OUT_SHR_LATCH	(1 << 4)
#define COMMHUB_GP_OUT_SHR_CLK		(1 << 8)

#define F_SYNC_RESPONSE		 0xAA55


retval_t commhub_sync(void);
retval_t commhub_writeRegisterNoSync(uint8_t address, uint16_t value);
retval_t commhub_readRegisterNoSync(uint8_t address, uint16_t *value);
retval_t commhub_writeRegister(uint8_t address, uint16_t value);
retval_t commhub_readRegister(uint8_t address, uint16_t *value);
retval_t commhub_orRegister(uint8_t address, uint16_t value);
retval_t commhub_andRegister(uint8_t address, uint16_t value);
retval_t commhub_connectUarts(uint8_t addr0, uint8_t addr1);
retval_t commhub_connectUartRx(uint8_t addr_sniffer, uint8_t addr_sniffed);
retval_t commhub_disconnectUartRx(uint8_t addr);
retval_t commhub_readCounter(uint64_t *counter_ticks);
#endif /* _COMMHUB_1500_H_ */
