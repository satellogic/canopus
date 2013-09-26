#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/logging.h>
#include <canopus/md5.h>
#include <canopus/math_utils.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/nanowheel.h>
#include <canopus/board/channels.h>
#include <string.h>

#define NANOWHEEL_CMD_PING								0xB0
#define NANOWHEEL_CMD_RESET								0xBB
#define NANOWHEEL_CMD_SET_VOLTAGE						0xB1
#define NANOWHEEL_CMD_SET_DIRECTION						0xB2
#define NANOWHEEL_CMD_COAST								0xB3
#define NANOWHEEL_CMD_BRAKE								0xB4
#define NANOWHEEL_CMD_FDIR_SET							0xB5
#define	NANOWHEEL_CMD_FDIR_RESET						0xBC
#define NANOWHEEL_CMD_SET_SPEED							0xB6
#define NANOWHEEL_CMD_TLM_SINGLE						0xB8
#define NANOWHEEL_CMD_TLM_LOG_START						0xB9
#define NANOWHEEL_CMD_TLM_LOG_DOWNLOAD					0xBA
#define NANOWHEEL_CMD_memory_MEMORY_READ				0x10
#define NANOWHEEL_CMD_MEMORY_WRITE						0x11
#define NANOWHEEL_CMD_MEMORY_CALL						0x12
#define NANOWHEEL_CMD_FLASH_WRITE_FROM_MEMORY			0x15

#define NANOWHEEL_ANSWER_CMD_OK							0xAA
#define NANOWHEEL_ANSWER_PARAM_OK						0x00
#define NANOWHEEL_ANSWER_PARAM_ERROR					0x01

#define NANOWHEEL_FDIR_PASSWORD							0xBACA

#define NANOWHEEL_DUMMY									0xFF /* SPI DUMMY */

#define PKT_CMD_SIZE            1
#define PKT_ADDR_SIZE_BROKEN    2   /* FIXME gera */
#define PKT_ADDR_SIZE_24        3
#define PKT_ADDR_SIZE_32        4
#define PKT_ADDR_SIZE_24_BROKEN 2
#define PKT_DUMMY_SIZE          1

retval_t nwheel_ping() {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_PING, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + PKT_DUMMY_SIZE);
	uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
	response = frame_get_u8_nocheck(&answer);

    log_report_fmt(LOG_WHEEL_VERBOSE, "NWHEEL ping answer:0x%02x\n", response);
	log_report_fmt(LOG_WHEEL, "NWHEEL ping: %s\n", NANOWHEEL_ANSWER_CMD_OK == response ? "SUCCES" : "ERROR");

    return NANOWHEEL_ANSWER_CMD_OK == response ? RV_SUCCESS : RV_ERROR;
}

retval_t nwheel_reset() {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_RESET, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + PKT_DUMMY_SIZE);
	uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
	response = frame_get_u8_nocheck(&answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL reset answer:0x%02x\n", response);

    return NANOWHEEL_ANSWER_CMD_OK == response ? RV_SUCCESS : RV_ERROR;
}

retval_t nwheel_brake() {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_BRAKE, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + PKT_DUMMY_SIZE);
	uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL brake SPI: %s\n", retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
	response = frame_get_u8_nocheck(&answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL brake answer:0x%02x\n", response);

    return NANOWHEEL_ANSWER_CMD_OK == response ? RV_SUCCESS : RV_ERROR;
}

retval_t nwheel_coast() {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_COAST, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + PKT_DUMMY_SIZE);
	uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL coast SPI: %s\n", retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
	response = frame_get_u8_nocheck(&answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL coast answer:0x%02x\n", response);

    return NANOWHEEL_ANSWER_CMD_OK == response ? RV_SUCCESS : RV_ERROR;
}

retval_t nwheel_set_voltage(uint16_t voltage) {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_SET_VOLTAGE, (voltage >> 8) & 0xFF, voltage & 0xFF, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + 2 + PKT_DUMMY_SIZE);
    uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL set_voltage(0x%04x) SPI: %s\n", voltage, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
    response = frame_get_u8_nocheck(&answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL set_voltage cmd:0x%02x\n", response);

	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer,1); // FIXME verify
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL set_voltage param:0x%02x\n", response);

		if (NANOWHEEL_ANSWER_PARAM_OK == response){
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

retval_t nwheel_set_speed(uint16_t speed) {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_SET_SPEED, (speed >> 8) & 0xFF, speed & 0xFF, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + 2 + PKT_DUMMY_SIZE);
    uint8_t response;
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL set_speed(0x%04x) SPI: %s\n", speed, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);
    response = frame_get_u8_nocheck(&answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL set_speed cmd:0x%02x\n", response);

	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer,1); // FIXME verify
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL set_speed param:0x%02x\n", response);

		if (NANOWHEEL_ANSWER_PARAM_OK == response){
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

retval_t nwheel_set_direction(const nwheel_direction_e direction) {
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_SET_DIRECTION, (uint8_t) direction, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + 1 + PKT_DUMMY_SIZE);
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL set_direction(%d) SPI: %s\n", (uint8_t)direction, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);

	frame_advance(&answer,1);

    // TODO debug
	if (NANOWHEEL_ANSWER_CMD_OK == frame_get_u8_nocheck(&answer)){
		if (NANOWHEEL_ANSWER_PARAM_OK == frame_get_u8_nocheck(&answer)){
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

retval_t nwheel_FDIR_set(const uint16_t config){
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_FDIR_SET, (config >> 8) & 0xFF, config & 0xFF, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + 2 + PKT_DUMMY_SIZE);
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL FDIR_set(0x%04x) SPI: %s\n", config, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);

	frame_advance(&answer,1);

    // TODO debug
	if (NANOWHEEL_ANSWER_CMD_OK == frame_get_u8_nocheck(&answer)){
		frame_advance(&answer,1);
		if (NANOWHEEL_ANSWER_PARAM_OK == frame_get_u8_nocheck(&answer)){
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

retval_t nwheel_FDIR_reset(){
	frame_t cmd    = DECLARE_FRAME_BYTES(NANOWHEEL_CMD_FDIR_RESET, (NANOWHEEL_FDIR_PASSWORD >> 8) & 0xFF, NANOWHEEL_FDIR_PASSWORD & 0xFF, NANOWHEEL_DUMMY);
	frame_t answer = DECLARE_FRAME_SPACE(PKT_CMD_SIZE + 2 + PKT_DUMMY_SIZE);
	retval_t rv;

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL FDIR_reset SPI: %s\n", retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);

	frame_advance(&answer,1);

    // TODO debug
	if (NANOWHEEL_ANSWER_CMD_OK == frame_get_u8_nocheck(&answer)){
		frame_advance(&answer,1);
		if (NANOWHEEL_ANSWER_PARAM_OK == frame_get_u8_nocheck(&answer)){
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

retval_t nwheel_TLM_get(const uint16_t mask, nwheel_tlm_t *tlm) {

	uint8_t cmd_buffer[PKT_CMD_SIZE + 2 + NANOWHEEL_TLM_MAX*2 + PKT_DUMMY_SIZE]; /* XXX > 1024 on stack */
	uint8_t answer_buffer[PKT_CMD_SIZE + 2 + NANOWHEEL_TLM_MAX*2 + PKT_DUMMY_SIZE];

	frame_t cmd	= DECLARE_FRAME(cmd_buffer);
	frame_t answer = DECLARE_FRAME(answer_buffer);

	retval_t rv;
	uint8_t sensorCount;
	uint8_t i, index=0;

	sensorCount = bitcount(mask & NANOWHEEL_TLM_ALL_MASK);

	frame_put_u8(&cmd, NANOWHEEL_CMD_TLM_SINGLE);
	frame_put_u8(&cmd, (mask&0xFF00)>>8);
	frame_put_u8(&cmd, (mask&0x00FF));

	for (i=0;i<sensorCount;i++){
		frame_put_u16(&cmd,0xFFFF);
	}

	frame_put_u8(&cmd, NANOWHEEL_DUMMY);

	frame_reset_for_reading(&cmd);

	frame_advance(&answer, cmd.size);
	frame_reset_for_reading(&answer);

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer,1);

	if (NANOWHEEL_ANSWER_CMD_OK == frame_get_u8_nocheck(&answer)){
		frame_advance(&answer,1);
		if (NANOWHEEL_ANSWER_PARAM_OK == frame_get_u8_nocheck(&answer)){
			log_report(LOG_WHEEL,"Correct TLM date received\n");

			for (i=0;i<sensorCount;i++){
				while((mask & (1<<index))==0) index++;
				tlm->values[index++] = frame_get_u16_nocheck(&answer);
			}

			return RV_SUCCESS;
		}else{
			log_report(LOG_WHEEL,"TLM Command failed\n");
			return RV_ILLEGAL;
		}
	}else{
		log_report(LOG_WHEEL,"TLM Command failed\n");
		return RV_ILLEGAL;
	}
}

#undef __builtin_bitcount

//===========================================================================================================================================

#define MEMORY_READ32_BUFFER_SIZE   32
static retval_t
nwheel_memory_read32(nwheel_addr_t nwheel_addr, void *obc_ptr, const uint16_t size)
{
#   define PKT_BUFLEN_SIZE 2
#   define MEMORY_READ_HEADER_SIZE		(PKT_CMD_SIZE + PKT_ADDR_SIZE_BROKEN + PKT_BUFLEN_SIZE + PKT_DUMMY_SIZE)
    int i;
    uint8_t response;
	uint8_t cmd_buffer[MEMORY_READ_HEADER_SIZE + MEMORY_READ32_BUFFER_SIZE]; /* XXX stack overflow :/ */
	uint8_t answer_buffer[MEMORY_READ_HEADER_SIZE + MEMORY_READ32_BUFFER_SIZE];

	frame_t cmd	= DECLARE_FRAME(cmd_buffer);
	frame_t answer = DECLARE_FRAME(answer_buffer);
	retval_t rv;

	if (size > MEMORY_READ32_BUFFER_SIZE) return RV_ILLEGAL;

	frame_put_u8(&cmd, NANOWHEEL_CMD_memory_MEMORY_READ);
	frame_put_u16(&cmd, nwheel_addr); /* XXX u24 */
	frame_put_u16(&cmd, size);
	frame_put_u8(&cmd, NANOWHEEL_DUMMY);
	for (i=0;i<size;i++) {
		frame_put_u8(&cmd, NANOWHEEL_DUMMY);
	}
	frame_reset_for_reading(&cmd);

    frame_advance(&answer, MEMORY_READ_HEADER_SIZE + size);
	frame_reset_for_reading(&answer);

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL mem_rd(0x%06x, 0x%08x, %d) SPI: %s\n", nwheel_addr, obc_ptr, size, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer, PKT_DUMMY_SIZE);
    response = frame_get_u8_nocheck(&answer);

    log_report_fmt(LOG_WHEEL, "NWHEEL mem_rd cmd:0x%02x\n", response);
	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer, 3); // FIXME document 3: 2+2=4?
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL mem_rd param:0x%02x\n", response);
		if (NANOWHEEL_ANSWER_PARAM_OK == response){
			rv = RV_SUCCESS;
			frame_get_data(&answer, obc_ptr, size);
		} else{
			rv = RV_ILLEGAL;
		}
		return rv;
	} else{
		return RV_ILLEGAL;
	}
}

retval_t
nwheel_memory_read(nwheel_addr_t nwheel_addr, void *obc_ptr, uint16_t size)
{
    retval_t rv;
    uint8_t *host_ptr = (uint8_t *)obc_ptr;

    while (0 != size) {
        uint16_t csize = size <= MEMORY_READ32_BUFFER_SIZE ? size : MEMORY_READ32_BUFFER_SIZE;
        rv = nwheel_memory_read32(nwheel_addr, host_ptr, csize);

        if (RV_SUCCESS != rv) {
            return rv;
        }
        nwheel_addr += MEMORY_READ32_BUFFER_SIZE;
        host_ptr += MEMORY_READ32_BUFFER_SIZE;
        size -= csize;
    }
    return RV_SUCCESS;
}

#define MEMORY_WRITE32_BUFFER_SIZE   32
static retval_t
nwheel_memory_write32(nwheel_addr_t nwheel_addr, const void *obc_ptr, const uint16_t size)
{
#   define MEMORY_WRITE_HEADER_SIZE		(PKT_CMD_SIZE + PKT_ADDR_SIZE_BROKEN + 1 + PKT_DUMMY_SIZE)
	frame_t cmd	   = DECLARE_FRAME_SPACE(MEMORY_WRITE_HEADER_SIZE + MEMORY_WRITE32_BUFFER_SIZE);
	frame_t answer = DECLARE_FRAME_SPACE(MEMORY_WRITE_HEADER_SIZE + MEMORY_WRITE32_BUFFER_SIZE);
	retval_t rv;
    uint8_t response;

	if (size > MEMORY_WRITE32_BUFFER_SIZE) return RV_ILLEGAL;

	frame_put_u8(&cmd, NANOWHEEL_CMD_MEMORY_WRITE);
	frame_put_u16(&cmd, nwheel_addr); /* XXX u24 */
	frame_put_u8(&cmd, size);
	frame_put_data(&cmd, obc_ptr, size);
    frame_reset(&cmd);

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL mem_wr(0x%06x, 0x%08x, %d) SPI: %s\n", nwheel_addr, obc_ptr, size, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	frame_advance(&answer, PKT_DUMMY_SIZE);
    response = frame_get_u8_nocheck(&answer);

    log_report_fmt(LOG_WHEEL, "NWHEEL mem_wr cmd:0x%02x\n", response);
	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer, MEMORY_WRITE_HEADER_SIZE - 2 + 32 - 1); /* FIXME uh? document... */
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL mem_wr param:0x%02x\n", response);
		if (NANOWHEEL_ANSWER_PARAM_OK == response) {
			rv = RV_SUCCESS;
		} else{
			rv = RV_ILLEGAL;
		}
		return rv;
	} else{
		return RV_ILLEGAL;
	}
}

retval_t
nwheel_memory_write(nwheel_addr_t nwheel_addr, const void *obc_ptr, uint16_t size) {
    retval_t rv;
    uint8_t *host_ptr = (uint8_t *)obc_ptr;

    while (0 != size) {
        uint16_t csize = size <= MEMORY_WRITE32_BUFFER_SIZE ? size : MEMORY_WRITE32_BUFFER_SIZE;
        rv = nwheel_memory_write32(nwheel_addr, host_ptr, csize);
        if (RV_SUCCESS != rv) {
            return rv;
        }
        nwheel_addr += MEMORY_WRITE32_BUFFER_SIZE;
        host_ptr += MEMORY_WRITE32_BUFFER_SIZE;
        size -= csize;
    }
    return RV_SUCCESS;
}

// XXX: This function need a double check, cant test it (Alan)
#define PKTCALL_PARAM_SIZE 2
retval_t nwheel_memory_call(const uint32_t nwheel_addr, uint16_t param1, uint16_t param2, uint8_t *reply) {
#   define MEMORY_CALL_HEADER_SIZE (PKT_CMD_SIZE + PKT_ADDR_SIZE_24 + PKTCALL_PARAM_SIZE + PKTCALL_PARAM_SIZE + PKT_DUMMY_SIZE)
	frame_t cmd	= DECLARE_FRAME_SPACE(MEMORY_CALL_HEADER_SIZE);
	frame_t answer = DECLARE_FRAME_SPACE(MEMORY_CALL_HEADER_SIZE);
	retval_t rv;
    uint8_t response;

	frame_put_u8(&cmd, NANOWHEEL_CMD_MEMORY_CALL);
	frame_put_u24(&cmd, nwheel_addr);
	frame_put_u16(&cmd, param1);
	frame_put_u16(&cmd, param2);
	frame_put_u8(&cmd, NANOWHEEL_DUMMY);
	frame_reset_for_reading(&cmd);

    frame_advance(&answer, MEMORY_CALL_HEADER_SIZE);
    frame_reset_for_reading(&answer);
    log_report_xxd(LOG_WHEEL, cmd.buf, cmd.size);

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL mem_call(0x%06x, 0x%4x, 0x%04x) SPI: %s\n", nwheel_addr, param1, param2, retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
    log_report_xxd(LOG_WHEEL, answer.buf, answer.size);
	frame_advance(&answer, PKT_DUMMY_SIZE);
    response = frame_get_u8_nocheck(&answer);

    log_report_fmt(LOG_WHEEL, "NWHEEL mem_call cmd:0x%02x\n", response);
	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer,PKT_ADDR_SIZE_24 + PKTCALL_PARAM_SIZE + PKTCALL_PARAM_SIZE-1);
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL mem_call param:0x%02x\n", response);
		if (NANOWHEEL_ANSWER_PARAM_OK == response){
			*reply = frame_get_u8_nocheck(&answer);
            log_report_fmt(LOG_WHEEL, "NWHEEL mem_call reply:0x%02x\n", *reply);
			return RV_SUCCESS;
		}else{
			return RV_ILLEGAL;
		}
	}else{
		return RV_ILLEGAL;
	}
}

// XXX: This function need a double check, cant test it (Alan)
retval_t
nwheel_flash_write(nwheel_addr_t nwheel_flash_addr /* to */, nwheel_addr_t nwheel_addr /* from */, uint16_t size)
{
#   define FLASH_WRITE_HEADER_SIZE (PKT_CMD_SIZE + PKT_ADDR_SIZE_24_BROKEN + PKT_ADDR_SIZE_24_BROKEN + PKT_BUFLEN_SIZE + PKT_DUMMY_SIZE)
	frame_t cmd    = DECLARE_FRAME_SPACE(FLASH_WRITE_HEADER_SIZE);
	frame_t answer = DECLARE_FRAME_SPACE(FLASH_WRITE_HEADER_SIZE-1);
	retval_t rv;
    uint8_t response;

	frame_put_u8(&cmd, NANOWHEEL_CMD_FLASH_WRITE_FROM_MEMORY);
	frame_put_u16(&cmd, nwheel_flash_addr); /* XXX u24 */
	frame_put_u16(&cmd, nwheel_addr); /* XXX u24 */
	frame_put_u16(&cmd, size);
//	frame_put_u8(&cmd, NANOWHEEL_DUMMY);
	frame_reset_for_reading(&cmd);
    log_report_xxd(LOG_WHEEL, cmd.buf, cmd.size);

    log_report_xxd(LOG_WHEEL, answer.buf, answer.size);

	rv = channel_transact(ch_nanowheel, &cmd, 0, &answer);
    log_report_fmt(LOG_WHEEL, "NWHEEL flash_wr(0x%06x, 0x%6x, %d) SPI: %s\n", nwheel_flash_addr, nwheel_addr, size, retval_s(rv));
    switch (rv) {
    case RV_PARTIAL: //ugly bug but this is working
    case RV_SUCCESS:
        break;
    default:
        return rv;
    }

	frame_reset_for_reading(&answer);
	frame_advance(&answer, 1);
    response = frame_get_u8_nocheck(&answer);

    log_report_fmt(LOG_WHEEL, "NWHEEL flash_wr cmd:0x%02x\n", response);
	if (NANOWHEEL_ANSWER_CMD_OK == response){
		frame_advance(&answer, 5 /* FIXME doc? */);
        response = frame_get_u8_nocheck(&answer);
        log_report_fmt(LOG_WHEEL, "NWHEEL flash_wr param:0x%02x\n", response);
        // CANT check the param value OK because of the writing delay
        vTaskDelay(100/portTICK_RATE_MS);
        return RV_SUCCESS;
	}else{
		return RV_ILLEGAL;
	}
}

#define FLASHWRITE_MAXSIZE 128 /* flash_write byte 511 broken */
#define MSP430_FLASHSECTOR_ALIGNED(addr) ((addr) & (FLASHWRITE_MAXSIZE - 1))

retval_t
nwheel_flash_write_1block_aligned(nwheel_addr_t nwheel_flash_addr /* to */, void *obc_bufptr /* from */, uint16_t size, nwheel_addr_t nwheel_ram_addr /* tmpbuf */)
{
    uint8_t *src = (uint8_t *)obc_bufptr;
    retval_t rv;

    if (MSP430_FLASHSECTOR_ALIGNED(nwheel_flash_addr)) {
        log_report_fmt(LOG_WHEEL,"NWHEEL flash write error. Trying to write at 0x%06x and max address permited is 0x%06x\n", nwheel_flash_addr, (FLASHWRITE_MAXSIZE - 1));
        return RV_ILLEGAL;
    }
    if (size > FLASHWRITE_MAXSIZE) {
    	log_report_fmt(LOG_WHEEL,"NWHEEL flash write error. Trying to write 0x%06x bytes and the max size permited is 0x%06x\n", size, FLASHWRITE_MAXSIZE);
        return RV_ILLEGAL;
    }

    log_report_fmt(LOG_WHEEL, "Filling NWHEEL 0x%06x (RAM) from OBC 0x%08x (size:%d)\n", nwheel_ram_addr, src, size);
    rv = nwheel_memory_write(nwheel_ram_addr, src, size);
    if (RV_SUCCESS != rv) {
        log_report_fmt(LOG_WHEEL, "ALGO FALLO: %s\n, ", retval_s(rv));
        return rv;
    }

    log_report_fmt(LOG_WHEEL, "Flashing NWHEEL 0x%06x from 0x%6x (size:%d)\n", nwheel_flash_addr, nwheel_ram_addr);
    rv = nwheel_flash_write(nwheel_flash_addr, nwheel_ram_addr, size);
    log_report_fmt(LOG_WHEEL, "--------------------------------------%s, ", retval_s(rv));

    if (RV_SUCCESS != rv) {
        log_report_fmt(LOG_WHEEL, "%s\n, ", retval_s(rv));
        return rv;
    }
    return rv;

}

retval_t
nwheel_flash_write_blocks_aligned(nwheel_addr_t nwheel_flash_addr /* to */, void *obc_bufptr /* from */, uint16_t size, nwheel_addr_t nwheel_ram_addr /* tmpbuf */)
{
    uint8_t *src = (uint8_t *)obc_bufptr;
    int i, count;
    retval_t rv, grv = RV_SUCCESS;

    if (nwheel_flash_addr & (FLASHWRITE_MAXSIZE - 1)) {
        return RV_ILLEGAL;
    }

    count = size / FLASHWRITE_MAXSIZE;
    if (size % FLASHWRITE_MAXSIZE) {
        count += 1;
    }

    for (i = 0; i < count; i++, nwheel_flash_addr += FLASHWRITE_MAXSIZE, src += FLASHWRITE_MAXSIZE, size -= FLASHWRITE_MAXSIZE) {

        rv = nwheel_flash_write_1block_aligned(nwheel_flash_addr, src, FLASHWRITE_MAXSIZE /* FIXME why can't we write < MAXSIZE? */, nwheel_ram_addr);
        log_report_fmt(LOG_WHEEL, "Flashing NWHEEL part %i: %s\n",i, retval_s(rv));
        if (RV_SUCCESS != rv) {
            grv = rv;
        }
    }

    return grv;
}

retval_t
nwheel_memory_md5(nwheel_addr_t nwheel_addr, uint16_t size, void *md5_dstptr)
{
    retval_t rv;
    MD5_CTX ctx;
    unsigned char buf[MEMORY_READ32_BUFFER_SIZE];

    assert(NULL != md5_dstptr);
    memset(md5_dstptr, 0, MD5_DIGEST_SIZE);

    MD5Init(&ctx);
    while (0 != size) {
        uint16_t csize = size <= MEMORY_READ32_BUFFER_SIZE ? size : MEMORY_READ32_BUFFER_SIZE;
        rv = nwheel_memory_read32(nwheel_addr, buf, csize);
        if (RV_SUCCESS != rv) {
            return rv;
        }
        nwheel_addr += MEMORY_READ32_BUFFER_SIZE;
        size -= csize;

        MD5Update(&ctx, buf, csize);
    }
    MD5Final(&ctx);

    memcpy(md5_dstptr, ctx.digest, MD5_DIGEST_SIZE);
    LOG_REPORT_MD5(LOG_WHEEL, "NWHEEL MD5: ", &ctx);

    return RV_SUCCESS;
}

retval_t nwheel_jump_to_high_version(){
	uint8_t buff;

	if (nwheel_ping()!=RV_SUCCESS){
		log_report(LOG_WHEEL, "nwheel ping Failed\n");
		return RV_ERROR;
	}
	log_report(LOG_WHEEL, "nwheel ping  OK\n");

	// Jump to new firmware
	nwheel_memory_call(0xE8AE,0,0,&buff);
	vTaskDelay(1000/portTICK_RATE_MS);

	if (nwheel_ping()!=RV_SUCCESS){
		log_report(LOG_WHEEL, "nwheel ping Failed\n");
		return RV_ERROR;
	}
	log_report(LOG_WHEEL," ------ Jumped to new firmware at 0x9000! ------ \n");

	return RV_SUCCESS;
}

retval_t nwheel_jump_to_low_version(){
	uint8_t buff[10];

	if (nwheel_ping()!=RV_SUCCESS){
		log_report(LOG_WHEEL, "nwheel ping Failed\n");
		return RV_ERROR;
	}
	log_report(LOG_WHEEL, "nwheel ping  OK\n");

	// Jump to new firmware
	nwheel_memory_call(0xE78A,0,0,buff);
	vTaskDelay(1000/portTICK_RATE_MS);

	if (nwheel_ping()!=RV_SUCCESS){
		log_report(LOG_WHEEL, "nwheel ping Failed\n");
		return RV_ERROR;
	}
	log_report(LOG_WHEEL," ------ Jumped to original firmware at 0x0000! ------ \n");

	return RV_SUCCESS;
}
