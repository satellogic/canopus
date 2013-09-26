#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/logging.h>
#include <canopus/drivers/flash.h>

#include "system.h"
#include "sys_core.h"

#include "F021.h"

#include "flash_f021.h"
#include "nvram_f021.h"

// TODO mirrored + swapped addresses

#define ROUNDUPCAST(f) ((unsigned int)(f + 0.5))

typedef struct sector_info_st {
    Fapi_FlashBankType bankId;
    Fapi_FlashSectorType sectorId;
    Fapi_FlashBankTechType bankTech;
    uint32_t sectorSize; /* in bytes */
    uint32_t sectorStartAddress;
} sector_info_t;

static const Fapi_FlashBankType // could be filled using Fapi_getDeviceInfo().u16NumberOfBanks
bank_ids[] = {
    // for TMS570LS3137
    Fapi_FlashBank0,
    Fapi_FlashBank1,
    Fapi_FlashBank7
};

static enum lock_e { UNLOCKED, LOCKED} lock = UNLOCKED;

static sector_info_t api_bank;

static Fapi_FlashBankType currentBankId = ~0;

retval_t
flash_lock() // maybe not necessary, but in case (see nvram)
{
	if (LOCKED == lock) {
		return RV_BUSY;
	}
	lock = LOCKED;

	return RV_SUCCESS;
}

void
flash_unlock()
{
	lock = UNLOCKED;
}

Fapi_StatusType
Fapi_serviceWatchdogTimer(void)
{
	FUTURE_HOOK_0(flash_serviceWatchdogTimer);

	return Fapi_Status_Success;
}

static boolean_t
get_sector_info(uint32_t addr, sector_info_t *info)
{
	int bank_id, sector_id;
	Fapi_StatusType status;
	Fapi_FlashBankSectorsType fbs;
	uint32_t sector_addr, asked_addr = (uint32_t)addr;
	uint32_t sector_size;

	for (bank_id = 0; bank_id < ARRAY_COUNT(bank_ids); bank_id++) {
		status = Fapi_getBankSectors(bank_ids[bank_id], &fbs);
		if (Fapi_Status_Success != status) {
			continue;
		}
		sector_addr = fbs.u32BankStartAddress;
		for (sector_id = 0; sector_id < fbs.u32NumberOfSectors; sector_id++) {
			if (Fapi_FLEE == fbs.oFlashBankTech) {
				sector_size = fbs.au16SectorSizes[0];
			} else {
				sector_size = fbs.au16SectorSizes[sector_id];
			}
			sector_size *= 1024;
			if (sector_addr <= asked_addr && asked_addr < sector_addr + sector_size) {
				info->bankTech = fbs.oFlashBankTech;
				info->bankId = bank_ids[bank_id];
				info->sectorId = sector_id;
				info->sectorStartAddress = sector_addr;
				info->sectorSize = sector_size;

				return true;
			}
			sector_addr += sector_size;
		}
	}

	return false;
}

flash_err_t
flash_init(void)
{
	flash_err_t rv;

	_coreDisableFlashEcc_(); // allowed by myself

#if 0
	//Fapi_initializeFlashBanks(ROUNDUPCAST(HCLK_FREQ));
	//while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
	//	////vTaskDelay(15 / portTICK_RATE_MS);
	//}
#endif

	// this will call Fapi_initializeFlashBanks()
    rv = flash_init_nvram();

    if (RV_SUCCESS == rv) {
        (void)get_sector_info((uint32_t)&flash_init, &api_bank);
    }

    return rv == RV_SUCCESS ? FLASH_ERR_OK : FLASH_ERR_HWR;
}

static flash_err_t
flash_erase_sectorinfo_locked(const sector_info_t *sinfo)
{
	Fapi_StatusType status;
	uint32_t fsm_status;
	Fapi_FlashStatusWordType poFlashStatusWord;

	if (api_bank.bankId == sinfo->bankId) {
		return FLASH_ERR_PROTECT;
	}
	//while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
	//	////vTaskDelay(15 / portTICK_RATE_MS);
	//}
	if (currentBankId != sinfo->bankId) {
		status = Fapi_setActiveFlashBank(sinfo->bankId);
		if (Fapi_Status_Success != status) {
			return FLASH_ERR_INVALID;
		}
		currentBankId = sinfo->bankId;
	}
	if (Fapi_FLEE == sinfo->bankTech) {
		status = Fapi_enableEepromBankSectors(1 << sinfo->sectorId, 0);
	} else {
		status = Fapi_enableMainBankSectors(1 << sinfo->sectorId);
	}
	if (Fapi_Status_Success != status) {
		return FLASH_ERR_INVALID;
	}

	while (Fapi_Status_FsmReady != FAPI_CHECK_FSM_READY_BUSY) {
		////vTaskDelay(15 / portTICK_RATE_MS);
	}

	status = Fapi_issueAsyncCommand(Fapi_ClearStatus);
	if (Fapi_Status_Success != status) {
		return FLASH_ERR_INVALID;
	}

	/* Wait for FSM to finish */
	while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
		//vTaskDelay(15 / portTICK_RATE_MS);
	}

	/* Check the FSM Status to see if there were no errors */
	fsm_status = FAPI_GET_FSM_STATUS;
	//

	log_report_fmt(LOG_FLASH, "flash_erase_sector(0x%08x, 0x%08x) EraseSector\r\n", sinfo->sectorStartAddress, sinfo->sectorSize);
	vPortEnterCritical();
	status = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (uint32_t *)sinfo->sectorStartAddress);
	//// do NOT use log_report*() here since we are running without IRQ
	if (Fapi_Status_Success != status) {
		vPortExitCritical();
		return FLASH_ERR_INVALID;
	}

	/* Wait for FSM to finish */
	while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
		//vTaskDelay(15 / portTICK_RATE_MS);
	}

	/* Check the FSM Status to see if there were no errors */
	fsm_status = FAPI_GET_FSM_STATUS;

	vPortExitCritical();
	/* black magic */
	Fapi_flushPipeline();

	if (fsm_status) {
		return FLASH_ERR_PROTOCOL;
	}

	status = Fapi_doBlankCheck((uint32_t *)sinfo->sectorStartAddress, sinfo->sectorSize / sizeof(uint32_t), &poFlashStatusWord);
	if (Fapi_Status_Success != status) {
		log_report_fmt(LOG_FLASH, "flash_erase_sector(0x%08x, 0x%08x) BlankCheck Fapi_Status:%d\r\n", sinfo->sectorStartAddress, sinfo->sectorSize, status);
		return FLASH_ERR_INVALID;
	}

	return FLASH_ERR_OK;
}

static flash_err_t
flash_erase_sectorinfo(const sector_info_t *sinfo)
{
	flash_err_t err;

	if (RV_SUCCESS != flash_lock()) {
		return FLASH_ERR_PROTOCOL;
	}
	err = flash_erase_sectorinfo_locked(sinfo);
	flash_unlock();

	return err;
}

flash_err_t
flash_erase_sector(const void *addr)
{
	sector_info_t sinfo;

	if (false == get_sector_info((uint32_t)addr, &sinfo)) {
		return FLASH_ERR_INVALID;
	}
	return flash_erase_sectorinfo(&sinfo);
}

flash_err_t // TODO rename _range
flash_erase(const void *base_block_addr, unsigned int size_in_bytes)
{
	uint32_t addr = (uint32_t)base_block_addr;
	uint32_t end_addr = (uint32_t)base_block_addr + size_in_bytes;
	sector_info_t sinfo;

	if (0 == size_in_bytes) {
		return FLASH_ERR_INVALID; // FIXME why? if we want to erase a whole sector it is convenient...
	}
	while (addr < end_addr) {
		if (false == get_sector_info(addr, &sinfo)) {
			return FLASH_ERR_INVALID;
		}
		if (FLASH_ERR_OK != flash_erase_sectorinfo(&sinfo)) {
			return FLASH_ERR_INVALID;
		}
		addr = sinfo.sectorStartAddress + sinfo.sectorSize;
	}

	return FLASH_ERR_OK;
}

#define WORDSIZE sizeof(uint32_t) // FIXME might go upto 128bits

static flash_err_t
flash_write_sector_locked(const void *addr, const void *data, int len_in_bytes) // TODO arg: err_addr
{
	Fapi_StatusType status;
	uint32_t fsm_status;
	sector_info_t p, *sinfo = &p;
	int bytes;
	uint32_t *paddr = (uint32_t *)addr; // TODO align?
	uint8_t *pdata = (uint8_t *)data;

#if 0
	if (!(((uint32_t)addr) & ~3)) {
		return FLASH_ERR_INVALID; // FIXME necessary?
	}
#endif
	if (false == get_sector_info((uint32_t)addr, &p)) {
		log_report_fmt(LOG_FLASH, "flash_write_sector ERROR unknown sector (addr:0x%08x)\n", (uint32_t)addr);
		return FLASH_ERR_INVALID;
	}

	if (api_bank.bankId == sinfo->bankId) {
		log_report_fmt(LOG_FLASH, "flash_write_sector ERROR api_bank.bankId=%d\n", api_bank.bankId);
		return FLASH_ERR_PROTECT;
	}
	//while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
	//	////vTaskDelay(15 / portTICK_RATE_MS);
	//}
	if (currentBankId != sinfo->bankId) {
		status = Fapi_setActiveFlashBank(sinfo->bankId);
		if (Fapi_Status_Success != status) {
			log_report_fmt(LOG_FLASH, "flash_write_sector ERROR Fapi_setActiveFlashBank(bankId:%d) status=%d\n", sinfo->bankId, status);
			return FLASH_ERR_INVALID;
		}
		currentBankId = sinfo->bankId;
	}
	if (Fapi_FLEE == sinfo->bankTech) {
		status = Fapi_enableEepromBankSectors(1 << sinfo->sectorId, 0);
	} else {
		status = Fapi_enableMainBankSectors(1 << sinfo->sectorId);
	}
	if (Fapi_Status_Success != status) {
		log_report_fmt(LOG_FLASH, "flash_write_sector ERROR Fapi_enable*BankSectors(sectorId:%d) status=%d\n", sinfo->sectorId, status);
		return FLASH_ERR_INVALID;
	}

	while (Fapi_Status_FsmReady != FAPI_CHECK_FSM_READY_BUSY) {
		////vTaskDelay(15 / portTICK_RATE_MS);
	}

	// --

	status = Fapi_issueAsyncCommand(Fapi_ClearStatus);
	if (Fapi_Status_Success != status) {
		return FLASH_ERR_INVALID;
	}

	/* Wait for FSM to finish */
	while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
		//vTaskDelay(15 / portTICK_RATE_MS);
	}

	/* Check the FSM Status to see if there were no errors */
	fsm_status = FAPI_GET_FSM_STATUS;
	//

	// --

	log_report_fmt(LOG_FLASH, "flash_write_sector(0x%08x, 0x%06x, src=0x%08x) Programming...\r\n", (uint32_t)addr, len_in_bytes, (uint32_t)data);
	vPortEnterCritical();
	while (len_in_bytes > 0) {
		if (len_in_bytes > WORDSIZE) {
			bytes = WORDSIZE;
		} else {
			bytes = len_in_bytes;
		}
		status = Fapi_issueProgrammingCommand(paddr, pdata, bytes, NULL, 0, Fapi_DataOnly);
		if (Fapi_Status_Success != status) {
			vPortExitCritical();
			log_report_fmt(LOG_FLASH, "flash_write_sector(0x%08x, still:0x%06x) ERROR1 Program Fapi_Status:%d\r\n", (uint32_t)paddr, len_in_bytes, status);
			return FLASH_ERR_INVALID;
		}

		/* Wait for FSM to finish */
		while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
			//vTaskDelay(15 / portTICK_RATE_MS);
		}

		/* Check the FSM Status to see if there were no errors */
		fsm_status = FAPI_GET_FSM_STATUS;
		if (fsm_status) { // see FMSTAT_BITS

			//if INVDAT:

			status = Fapi_issueAsyncCommand(Fapi_ClearStatus);
			if (Fapi_Status_Success != status) {
				vPortExitCritical();
				log_report_fmt(LOG_FLASH, "flash_write_sector(0x%08x, still:0x%06x) ERROR2 fsm_status:%d Fapi_Status:%d\r\n", (uint32_t)paddr, len_in_bytes, fsm_status, status);
				return FLASH_ERR_INVALID;
			}

			/* Wait for FSM to finish */
			while (Fapi_Status_FsmBusy == FAPI_CHECK_FSM_READY_BUSY) {
				//vTaskDelay(15 / portTICK_RATE_MS);
			}

			/* Check the FSM Status to see if there were no errors */
			fsm_status = FAPI_GET_FSM_STATUS;

			vPortExitCritical();
			log_report_fmt(LOG_FLASH, "flash_write_sector(0x%08x, still:0x%06x) ERROR3 fsm_status:%d\r\n", (uint32_t)paddr, len_in_bytes, fsm_status);
			return FLASH_ERR_PROTOCOL;
		}
		len_in_bytes -= bytes;
		pdata += bytes;
		paddr++;
	}
	Fapi_flushPipeline();
	vPortExitCritical();
	log_report_fmt(LOG_FLASH, "flash_write_sector(0x%08x, src=0x%08x) Done.\r\n", (uint32_t)addr, (uint32_t)data);

	return FLASH_ERR_OK;
}

flash_err_t
flash_write(const void *addr, const void *data, int len_in_bytes)
{
	flash_err_t err;

	if (RV_SUCCESS != flash_lock()) {
		return FLASH_ERR_PROTOCOL;
	}
	err = flash_write_sector_locked(addr, data, len_in_bytes);
	flash_unlock();

	return err;
}

size_t
flash_sector_wordsize(const void *addr)
{
    return sizeof(uint16_t); // FIXME 128b?
}

size_t
flash_sector_size(const void *addr)
{
	sector_info_t sinfo;

	if (false == get_sector_info((uint32_t)addr, &sinfo)) {
		return 0;
	}

	return sinfo.sectorSize;
}

#pragma RETAIN ( Fapi_not_yet_used_funclist )
const void *Fapi_not_yet_used_funclist[] = {
		&Fapi_isAddressEcc,
		&Fapi_remapEccAddress,
		&Fapi_remapMainAddress,
		&Fapi_issueProgrammingCommandForEccAddresses,
		&Fapi_doBlankCheckByByte,
		&Fapi_doVerify,
		&Fapi_doVerifyByByte,
		&Fapi_doPsaVerify,
		&Fapi_calculatePsa,
		&Fapi_doMarginRead,
		&Fapi_doMarginReadByByte,
		&Fapi_getLibraryInfo,
		&Fapi_getDeviceInfo,
		&Fapi_calculateFletcherChecksum,
		&Fapi_calculateEcc
};
