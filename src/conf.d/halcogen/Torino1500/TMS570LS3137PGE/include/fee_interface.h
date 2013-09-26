/**********************************************************************************************************************
 *  COPYRIGHT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *                 TEXAS INSTRUMENTS INCORPORATED PROPRIETARY INFORMATION
 *
 *                 Property of Texas Instruments, Unauthorized reproduction and/or distribution
 *                 is strictly prohibited.  This product  is  protected  under  copyright  law
 *                 and  trade  secret law as an  unpublished work.
 *                 (C) Copyright Texas Instruments - 2011.  All rights reserved.
 *
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *         File:  fee_interface.h
 *      Project:  Tms570_TIFEEDriver
 *       Module:  FEE Driver
 *    Generator:  None
 *
 *  Description:  This file is interfce between Autosar FEE and TI FEE.
 *---------------------------------------------------------------------------------------------------------------------
 * Author:  Vishwanath Reddy
 *---------------------------------------------------------------------------------------------------------------------
 * Revision History
 *---------------------------------------------------------------------------------------------------------------------
 * Version        Date         Author               Change ID        Description
 *---------------------------------------------------------------------------------------------------------------------
 * 00.01.00       07Sept2012    Vishwanath Reddy     0000000000000    Initial Version
 * 00.01.01       14Sept2012    Vishwanath Reddy     0000000000000    Review changes
 * 00.01.02       30Nov2012     Vishwanath Reddy     SDOCM00097786    Misra Fixes, Memory segmentation changes.
 * 00.01.03       14Jan2013     Vishwanath Reddy     SDOCM00098510    Changes as requested by Vector.
 * 00.01.06		  11Mar2013	  	Vishwanath Reddy     SDOCM00099152    Added feature : copying of unconfigured blocks. 
 *********************************************************************************************************************/
   
#ifndef FEE_INTERFACE_H
#define FEE_INTERFACE_H


#include "ti_fee_cfg.h"

#if(TI_FEE_DRIVER == 0U) /* Include following macros only in Autosar Context */
#include "fee_cfg.h"
#include "nvm.h" 

#define Fee_None 0x00U	/* Take no action on single bit errors, (respond with corrected data), */
						/* return error for uncorrectable error reads (multi bit errors for ECC or parity failures). */
						/* For devices with no ECC (they may have parity or not) the only valid option is none. */
#define Fee_Fix 0x01U	/* single bit "zero" error will be fixed by reprogramming, single bit "one" error */
						/* will be fixed by marking the current entry as invalid and copying the data to a new entry, */
						/* return error for uncorrectable error reads (multi bit errors for ECC or parity failures). */

#define TI_Fee_None 0x00U	/* Take no action on single bit errors, (respond with corrected data), */
							/* return error for uncorrectable error reads (multi bit errors for ECC or parity failures). */
							/* For devices with no ECC (they may have parity or not) the only valid option is none. */
#define TI_Fee_Fix 0x01U	/* single bit "zero" error will be fixed by reprogramming, single bit "one" error */
							/* will be fixed by marking the current entry as invalid and copying the data to a new entry, */
							/* return error for uncorrectable error reads (multi bit errors for ECC or parity failures). */
							

#if(FEE_FLASH_ERROR_CORRECTION_HANDLING == Fee_Fix)
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - TI_Fee_Fix is a symbolic constant."*/
#define TI_FEE_FLASH_ERROR_CORRECTION_HANDLING 	TI_Fee_Fix
#else
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - TI_Fee_None is a symbolic constant."*/
#define TI_FEE_FLASH_ERROR_CORRECTION_HANDLING 	TI_Fee_None
#endif

/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_MAXIMUM_BLOCKING_TIME is a symbolic constant"*/
#define TI_FEE_MAXIMUM_BLOCKING_TIME 			FEE_MAXIMUM_BLOCKING_TIME
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_OPERATING_FREQUENCY is a symbolic constant."*/
#define TI_FEE_OPERATING_FREQUENCY 				FEE_OPERATING_FREQUENCY
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_FLASH_ERROR_CORRECTION_ENABLE is a symbolic constant."*/
#define TI_FEE_FLASH_ERROR_CORRECTION_ENABLE 	FEE_FLASH_ERROR_CORRECTION_ENABLE
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_FLASH_CRC_ENABLE is a symbolic constant."*/
#define TI_FEE_FLASH_CRC_ENABLE					FEE_FLASH_CRC_ENABLE
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_FLASH_WRITECOUNTER_SAVE is a symbolic constant."*/
#define TI_FEE_FLASH_WRITECOUNTER_SAVE 			FEE_FLASH_WRITECOUNTER_SAVE
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - NVM_DATASET_SELECTION_BITS is a symbolic constant."*/
#define TI_FEE_DATASELECT_BITS 					NVM_DATASET_SELECTION_BITS
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_NUMBER_OF_EEPS is a symbolic constant."*/
#define TI_FEE_NUMBER_OF_EEPS  					FEE_NUMBER_OF_EEPS
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_INDEX is a symbolic constant."*/
#define TI_FEE_INDEX                    		FEE_INDEX
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_PAGE_OVERHEAD is a symbolic constant."*/
#define TI_FEE_PAGE_OVERHEAD            		FEE_PAGE_OVERHEAD
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_BLOCK_OVERHEAD is a symbolic constant."*/
#define TI_FEE_BLOCK_OVERHEAD           		FEE_BLOCK_OVERHEAD
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_VIRTUAL_PAGE_SIZE is a symbolic constant."*/
#define TI_FEE_VIRTUAL_PAGE_SIZE        		FEE_VIRTUAL_PAGE_SIZE
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_VIRTUAL_SECTOR_OVERHEAD is a symbolic constant."*/
#define TI_FEE_VIRTUAL_SECTOR_OVERHEAD  		FEE_VIRTUAL_SECTOR_OVERHEAD
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_VIRTUAL_SECTOR_OVERHEAD is a symbolic constant."*/
#define TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY  		FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY

/*----------------------------------------------------------------------------*/
/* Virtual Sector Configuration                                               */

/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_NUMBER_OF_VIRTUAL_SECTORS is a symbolic constant."*/
/*SAFETYMCUSW 61 X MR:1.4,5.1 <INSPECTED> "Reason - Similar Identifier name is required here."*/
#define TI_FEE_NUMBER_OF_VIRTUAL_SECTORS 		FEE_NUMBER_OF_VIRTUAL_SECTORS
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1 is a symbolic constant."*/
/*SAFETYMCUSW 384 S MR:1.4,5.1 <INSPECTED> "Reason - Similar Identifier name is required here."*/
/*SAFETYMCUSW 61 X MR:1.4,5.1 <INSPECTED> "Reason - Similar Identifier name is required here."*/
#define TI_FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1	FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1


/*----------------------------------------------------------------------------*/
/* Block Configuration                                                        */
/*SAFETYMCUSW 79 S MR:19.4 <INSPECTED> "Reason - FEE_NUMBER_OF_BLOCKS is a symbolic constant."*/
#define TI_FEE_NUMBER_OF_BLOCKS  				FEE_NUMBER_OF_BLOCKS


#endif	/* TI_FEE_DRIVER */

#endif /* FEE_INTERFACE_H */
/**********************************************************************************************************************
 *  END OF FILE: fee_interface.h
 *********************************************************************************************************************/
