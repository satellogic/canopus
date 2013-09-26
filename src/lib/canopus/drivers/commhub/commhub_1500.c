#include <canopus/types.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/commhub_1500.h>
#include <canopus/frame.h>

#define CMD_FUNCTION_MASK 	0x7000
#define CMD_ADDRESS_MASK    0x3FFF

#define CMD_F_WRITE         0x0000
#define CMD_F_READ          0x4000
#define CMD_F_OR            0x1000 // not implemented
#define CMD_F_AND           0x2000 // not implemented
#define CMD_F_SYNC_H		 0xF0
#define CMD_F_SYNC_L		 0x11

#define F_RESYNC_TIMES		5
#define FPGA_DUMMY			0xA55A

#define COMMHUB_OFFSET_REG_LITHIUM 			(0*4)
#define COMMHUB_OFFSET_REG_CONDOR 			(1*4)
#define COMMHUB_OFFSET_REG_SVIP_0 			(2*4)
#define COMMHUB_OFFSET_REG_SVIP_1 			(3*4)
#define COMMHUB_OFFSET_REG_SPOT 			(0*4)
#define COMMHUB_OFFSET_REG_UMB 				(1*4)
#define COMMHUB_OFFSET_REG_TMS0_UART_0 		(0*4)
#define COMMHUB_OFFSET_REG_TMS0_UART_1 		(1*4)
#define COMMHUB_OFFSET_REG_OVERO_UART_1 	(0*4)

static const uint8_t commhub_uarts_address_mapping[] = {
		[COMMHUB_UART_ADDR_LITHIUM]			= COMMHUB_REG_UART_OTHER0_RX,
		[COMMHUB_UART_ADDR_CONDOR]			= COMMHUB_REG_UART_OTHER0_RX,
		[COMMHUB_UART_ADDR_SVIP_0]			= COMMHUB_REG_UART_OTHER0_RX,
		[COMMHUB_UART_ADDR_SVIP_1]			= COMMHUB_REG_UART_OTHER0_RX,
		[COMMHUB_UART_ADDR_SPOT]			= COMMHUB_REG_UART_OTHER1_RX,
		[COMMHUB_UART_ADDR_UMB]				= COMMHUB_REG_UART_OTHER1_RX,
		[COMMHUB_UART_ADDR_TMS_0_UART_0]	= COMMHUB_REG_UART_TMS_RX,
		[COMMHUB_UART_ADDR_TMS_0_UART_1]	= COMMHUB_REG_UART_TMS_RX,
		[COMMHUB_UART_ADDR_OVERO_UART_1]	= COMMHUB_REG_UART_OVERO_RX,
};

static const uint8_t commhub_uarts_offset_mapping[] = {
		[COMMHUB_UART_ADDR_LITHIUM]			= COMMHUB_OFFSET_REG_LITHIUM,
		[COMMHUB_UART_ADDR_CONDOR]			= COMMHUB_OFFSET_REG_CONDOR,
		[COMMHUB_UART_ADDR_SVIP_0]			= COMMHUB_OFFSET_REG_SVIP_0,
		[COMMHUB_UART_ADDR_SVIP_1]			= COMMHUB_OFFSET_REG_SVIP_1,
		[COMMHUB_UART_ADDR_SPOT]			= COMMHUB_OFFSET_REG_SPOT,
		[COMMHUB_UART_ADDR_UMB]				= COMMHUB_OFFSET_REG_UMB,
		[COMMHUB_UART_ADDR_TMS_0_UART_0]	= COMMHUB_OFFSET_REG_TMS0_UART_0,
		[COMMHUB_UART_ADDR_TMS_0_UART_1]	= COMMHUB_OFFSET_REG_TMS0_UART_1,
		[COMMHUB_UART_ADDR_OVERO_UART_1]	= COMMHUB_OFFSET_REG_OVERO_UART_1,
};


retval_t commhub_sync(){
	retval_t rv;
	frame_t cmd    = DECLARE_FRAME_BYTES(CMD_F_SYNC_H, CMD_F_SYNC_L);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	uint16_t response;
	uint8_t i;

	for (i=0; i < F_RESYNC_TIMES; ++i){
		frame_reset(&cmd);
		frame_reset(&answer);
		rv = channel_transact(ch_fpga_ctrl, &cmd, 0, &answer);
		SUCCESS_OR_RETURN(rv);
		frame_reset_for_reading(&answer);
		frame_get_u16(&answer, &response);
		if (response == F_SYNC_RESPONSE) {
			return RV_SUCCESS;
		}
	}
	return RV_TIMEOUT;
}


retval_t commhub_writeRegisterNoSync(uint8_t address, uint16_t value) {
	frame_t cmd    = DECLARE_FRAME_SPACE(4);
	frame_t answer = DECLARE_FRAME_SPACE(4);

	frame_put_u16(&cmd, CMD_F_WRITE | address );
	frame_put_u16(&cmd, value);
	frame_reset_for_reading(&cmd);

	return channel_transact(ch_fpga_ctrl, &cmd, 0, &answer);
}

retval_t commhub_writeRegister(uint8_t address, uint16_t value) {
	retval_t rv;

	rv = commhub_sync();
	if (RV_SUCCESS != rv) return rv;

	return commhub_writeRegisterNoSync(address, value);
}

retval_t commhub_readRegisterNoSync(uint8_t address, uint16_t *value) {
	retval_t rv;
	frame_t cmd    = DECLARE_FRAME_SPACE(4);
	frame_t answer = DECLARE_FRAME_SPACE(4);
	uint16_t dummy;

	frame_put_u16(&cmd, CMD_F_READ | address);
	frame_put_u16(&cmd, FPGA_DUMMY);
	frame_reset_for_reading(&cmd);

	rv = channel_transact(ch_fpga_ctrl, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_get_u16(&answer, &dummy);
	return frame_get_u16(&answer, value);
}

retval_t commhub_readRegister(uint8_t address, uint16_t *value) {
	retval_t rv;

	rv = commhub_sync();
	if (RV_SUCCESS != rv) return rv;

	return commhub_readRegisterNoSync(address, value);
}



retval_t commhub_orRegister(uint8_t address, uint16_t value) {
	uint16_t currentValue;
	retval_t rv;
	rv = commhub_sync();
	if (RV_SUCCESS != rv) return rv;

	rv = commhub_readRegisterNoSync(address, &currentValue);
	if (RV_SUCCESS != rv) return rv;

	currentValue |= value;
	return commhub_writeRegisterNoSync(address, currentValue);
}

retval_t commhub_andRegister(uint8_t address, uint16_t value) {
	uint16_t currentValue;
	retval_t rv;

	rv = commhub_sync();
	if (RV_SUCCESS != rv) return rv;

	rv = commhub_readRegisterNoSync(address, &currentValue);
	if (RV_SUCCESS != rv) return rv;

	currentValue &= value;
	return commhub_writeRegisterNoSync(address, currentValue);
}


static
retval_t _connectUarts(uint8_t addr0, uint8_t addr1){
	uint16_t value;
	retval_t rv;
	rv = commhub_readRegister(commhub_uarts_address_mapping[addr0], &value);
	SUCCESS_OR_RETURN(rv);
	value &= ~(0xf << commhub_uarts_offset_mapping[addr0]);
	value |= (addr1 << commhub_uarts_offset_mapping[addr0]);
	return commhub_writeRegister(commhub_uarts_address_mapping[addr0], value);
}

retval_t commhub_connectUarts(uint8_t addr0, uint8_t addr1){
	retval_t rv;
	rv = _connectUarts(addr0, addr1);
	SUCCESS_OR_RETURN(rv);
	return _connectUarts(addr1, addr0);
}

retval_t commhub_connectUartRx(uint8_t addr_sniffer, uint8_t addr_sniffed){
	return _connectUarts(addr_sniffer, addr_sniffed);
}

retval_t commhub_disconnectUartRx(uint8_t addr){
	return _connectUarts(addr, COMMHUB_UART_ADDR_HIGH);
}

static retval_t commhub_latchCounter() {
	retval_t rv;

	rv = commhub_writeRegister(COMMHUB_REG_PSW, 1);
	SUCCESS_OR_RETURN(rv);
	rv = commhub_writeRegister(COMMHUB_REG_PSW, 0);
	return rv;
}

retval_t commhub_readCounter(uint64_t *counter_ticks) {
	retval_t rv;
	uint16_t counterPart;
	uint64_t answer;

	rv = commhub_latchCounter();
	SUCCESS_OR_RETURN(rv);

	rv = commhub_readRegister(COMMHUB_REG_COUNTER_3, &counterPart);
	SUCCESS_OR_RETURN(rv);
	answer = counterPart;

	rv = commhub_readRegister(COMMHUB_REG_COUNTER_2, &counterPart);
	SUCCESS_OR_RETURN(rv);
	answer <<= 16;
	answer += counterPart;

	rv = commhub_readRegister(COMMHUB_REG_COUNTER_1, &counterPart);
	SUCCESS_OR_RETURN(rv);
	answer <<= 16;
	answer += counterPart;

	rv = commhub_readRegister(COMMHUB_REG_COUNTER_0, &counterPart);
	SUCCESS_OR_RETURN(rv);
	answer <<= 16;
	answer += counterPart;

	*counter_ticks = answer;
	return RV_SUCCESS;
}
