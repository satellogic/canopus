#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/nvram.h>

#include <string.h>

#include "ti_fee.h"
#include "ti_fee_types.h" // shut up CODAN

#include "flash_f021.h"
#include "nvram_f021.h"

#define TI_Fee_Task TI_Fee_MainFunction

static bool nvram_drv_initialized = false;

retval_t
flash_init_nvram(void) {
    TI_FeeModuleStatusType status;

    assert(false == nvram_drv_initialized);
    nvram_drv_initialized = true;

    TI_Fee_Init();
    status = TI_Fee_GetStatus(NVRAM_EEP_INDEX);
    assert(UNINIT != status);

    return IDLE == status ? RV_SUCCESS : RV_ERROR;
}

static retval_t
nvram_read_blocking_locked(void *addr, size_t size)
{
    Std_ReturnType rv;
    TI_FeeJobResultType jr;
    uint8* DataBufferPtr = (uint8 *)addr;
    uint16 Length = size;

    assert(true == nvram_drv_initialized);
    memset(DataBufferPtr, 0, Length);

    // finish previous
    while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
    	TI_Fee_Task();
    	//vTaskDelay(10 / portTICK_RATE_MS);
    }

    for (;;) {
    	rv = TI_Fee_Read(NVRAM_BLOCK_NUMBER, 0/*BlockOffset*/, DataBufferPtr, Length); // Synchronous
    	if (E_OK == rv) break;
        while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
        	TI_Fee_Task();
        	//vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
    for (;;) {
        jr = TI_Fee_GetJobResult(NVRAM_EEP_INDEX);
        switch (jr) {
        case JOB_PENDING:
            TI_Fee_Task();
            //vTaskDelay(10 / portTICK_RATE_MS);
            continue;
        case JOB_OK:
            return RV_SUCCESS;
        case JOB_FAILED:
        case BLOCK_INCONSISTENT:
        case BLOCK_INVALID:
            TI_Fee_Cancel(NVRAM_EEP_INDEX);
        case JOB_CANCELLED:
            return RV_ERROR;
        }
    }
}

static retval_t
nvram_write_blocking_locked(const void *addr, size_t size)
{
    Std_ReturnType rv;
    TI_FeeJobResultType jr;
    uint8_t buf[NVRAM_BLOCK_SIZE];
    uint8* DataBufferPtr = (uint8 *)addr;
    uint16 Length = size;

    memset(buf, 0xff, sizeof(buf));
    memcpy(buf, DataBufferPtr, Length);

    // finish previous
    while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
    	TI_Fee_Task();
    	//vTaskDelay(10 / portTICK_RATE_MS);
    }

    for (;;) {
    	rv = TI_Fee_WriteAsync(NVRAM_BLOCK_NUMBER, buf);
    	if (E_OK == rv) break;
        while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
        	TI_Fee_Task();
        	//vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
    for (;;) {
        jr = TI_Fee_GetJobResult(NVRAM_EEP_INDEX);
        switch (jr) {
        case JOB_PENDING:
	        TI_Fee_Task();
	        //vTaskDelay(10 / portTICK_RATE_MS);
	        continue;
        case JOB_OK:
	        return RV_SUCCESS;
        case JOB_FAILED:
        case BLOCK_INCONSISTENT:
        case BLOCK_INVALID:
	        TI_Fee_Cancel(NVRAM_EEP_INDEX);
        case JOB_CANCELLED:
    	    return RV_ERROR;
        }
    }
}

retval_t
nvram_read_blocking(void *addr, size_t size)
{
	retval_t rv;

	if (RV_SUCCESS != flash_lock()) {
		return RV_BUSY;
	}
	rv = nvram_read_blocking_locked(addr, size);
	flash_unlock();

	return rv;

}

retval_t
nvram_write_blocking(const void *addr, size_t size)
{
	retval_t rv;

	if (RV_SUCCESS != flash_lock()) {
		return RV_BUSY;
	}
	rv = nvram_write_blocking_locked(addr, size);
	flash_unlock();

	return rv;

}

static retval_t
nvram_erase_blocking_locked()
{

    Std_ReturnType rv;
    TI_FeeJobResultType jr;

    // finish previous
    while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
    	TI_Fee_Task();
    	//vTaskDelay(10 / portTICK_RATE_MS);
    }

    for (;;) {
    	rv = TI_Fee_EraseImmediateBlock(NVRAM_BLOCK_NUMBER);
    	if (E_OK == rv) break;
        while (IDLE != TI_Fee_GetStatus(NVRAM_EEP_INDEX)) {
        	TI_Fee_Task();
        	//vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
    for (;;) {
        jr = TI_Fee_GetJobResult(NVRAM_EEP_INDEX);
        switch (jr) {
        case JOB_PENDING:
	        TI_Fee_Task();
	        //vTaskDelay(10 / portTICK_RATE_MS);
	        continue;
        case JOB_OK:
	        return RV_SUCCESS;
        case JOB_FAILED:
        case BLOCK_INCONSISTENT:
        case BLOCK_INVALID:
	        TI_Fee_Cancel(NVRAM_EEP_INDEX);
        case JOB_CANCELLED:
    	    return RV_ERROR;
        }
    }
}

retval_t
nvram_erase_blocking()
{
	retval_t rv;

	if (RV_SUCCESS != flash_lock()) {
		return RV_BUSY;
	}
	rv = nvram_erase_blocking_locked();
	flash_unlock();

	return rv;

}

retval_t
nvram_format_blocking(uint32_t format_key)
{
	boolean rv;

	if (RV_SUCCESS != flash_lock()) {
		return RV_BUSY;
	}
	rv = TI_Fee_Format(format_key);
	flash_unlock();

	return rv == FALSE ? RV_SUCCESS : RV_ERROR;
}
