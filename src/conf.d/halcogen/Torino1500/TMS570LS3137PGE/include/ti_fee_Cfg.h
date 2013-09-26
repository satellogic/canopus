/**********************************************************************************************************************
 *  COPYRIGHT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *                 TEXAS INSTRUMENTS INCORPORATED PROPRIETARY INFORMATION
 *
 *                 Property of Texas Instruments, Unauthorized reproduction and/or distribution
 *                 is strictly prohibited.  This product  is  protected  under  copyright  law
 *                 and  trade  secret law as an  unpublished work.
 *                 (C) Copyright Texas Instruments - 2012.  All rights reserved.
 *
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *         File:  ti_fee_cfg.h
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  HALcogen
 *
 *  Description:  This file implements the TI FEE Api.
 *---------------------------------------------------------------------------------------------------------------------
 * Author:  Vishwanath Reddy
 *---------------------------------------------------------------------------------------------------------------------
 * Revision History
 *---------------------------------------------------------------------------------------------------------------------
 * Version        Date         Author               Change ID        Description
 *---------------------------------------------------------------------------------------------------------------------
 * 03.00.00       31Aug2012    Vishwanath Reddy     0000000000000    Initial Version
 *
 *********************************************************************************************************************/

 
 #ifndef TI_FEE_CFG_H
 #define TI_FEE_CFG_H

#include "system.h" // HCLK_FREQ

/** @def TI_FEE_DRIVER
*   @brief Alias name for FEE Device 
*/
#define TI_FEE_DRIVER                                      1U

/* TI FEE General Configuration */
/** @def FEE_DEV_ERROR_DETECT
*   @brief Alias name for FEE Device Error
*/
#define TI_FEE_DEV_ERROR_DETECT                            STD_OFF

/** @def FEE_POLLING_MODE
*   @brief Alias name for FEE Polling Mode
*/
#define TI_FEE_POLLING_MODE                                STD_ON

/** @def FEE_OPERATING_FREQUENCY
*   @brief Alias name for FEE Operating Frequency
*/
#define ROUNDUPCAST(f) ((unsigned int)(f + 0.5))
#define TI_FEE_OPERATING_FREQUENCY                         ROUNDUPCAST(HCLK_FREQ)

/** @def FEE_FLASH_ERROR_CORRECTION_ENABLE
*   @brief Alias name for FEE Error Correction Enable
*/
#define TI_FEE_FLASH_ERROR_CORRECTION_ENABLE               STD_OFF 

/** @def FEE_FLASH_ERROR_CORRECTION_HANDLING
*   @brief Alias name for FEE Error Correction Handle
*/
#define TI_FEE_FLASH_ERROR_CORRECTION_HANDLING             TI_Fee_None 

/** @def FEE_FLASH_CRC_ENABLE
*   @brief Alias name for FEE CRC Enable
*/
#define TI_FEE_FLASH_CRC_ENABLE					           STD_OFF

/** @def FEE_FLASH_WRITECOUNTER_SAVE
*   @brief Alias name for FEE Flash Write Counter
*/
#define TI_FEE_FLASH_WRITECOUNTER_SAVE 			           STD_ON 

/** @def FEE_NUMBER_OF_EEPS
*   @brief Alias name for FEE EEPS
*/
#define TI_FEE_NUMBER_OF_EEPS  					           1U

/** @def TI_FEE_DATASELECT_BITS
*   @brief Alias name for FEE Data Select
*/
#define TI_FEE_DATASELECT_BITS                             0U

/** @def FEE_INDEX
*   @brief Alias name for FEE Index
*/
#define TI_FEE_INDEX                                       0U

/** @def FEE_PAGE_OVERHEAD
*   @brief Alias name for FEE Page Overhead
*/
#define TI_FEE_PAGE_OVERHEAD                               0U

/** @def FEE_BLOCK_OVERHEAD
*   @brief Alias name for FEE Block Overhead
*/
#define TI_FEE_BLOCK_OVERHEAD                              24U

/** @def FEE_VIRTUAL_PAGE_SIZE
*   @brief Alias name for FEE Virtual Page Size
*/
#define TI_FEE_VIRTUAL_PAGE_SIZE                           8U

/** @def FEE_VIRTUAL_SECTOR_OVERHEAD
*   @brief Alias name for FEE Virtual Sector Overhead
*/
#define TI_FEE_VIRTUAL_SECTOR_OVERHEAD                     16U

/** @def FEE_MAXIMUM_BLOCKING_TIME
*   @brief Alias name for FEE Maximum Blocking Time
*/
#define TI_FEE_MAXIMUM_BLOCKING_TIME                       600U

/* TI FEE Virtual Sector Configuration */

/** @def FEE_NUMBER_OF_VIRTUAL_SECTORS
*   @brief Alias name for FEE Number Of Virtual Sectors
*/
#define TI_FEE_NUMBER_OF_VIRTUAL_SECTORS                   2U  

/** @def FEE_NUMBER_OF_VIRTUAL_SECTORS
*   @brief Alias name for FEE Number Of Virtual Sectors for EEP1
*/
#define TI_FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1              0U

/* TI FEE Block Configuration */

/** @def FEE_NUMBER_OF_BLOCKS
*   @brief Alias name for FEE Number Of Blocks
*/
#define TI_FEE_NUMBER_OF_BLOCKS                            1U

/** @def TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY
*   @brief Alias name for Fee Number Of Unconfigured Blocks To Copy
*/
#define TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY          0U

/** @def TI_FEE_NUMBER_OF_EIGHTBYTEWRITES
*   @brief Alias name for Fee Number Of eight byte writes
*/
#define TI_FEE_NUMBER_OF_EIGHTBYTEWRITES          1U

 #endif /* TI_FEE_CFG_H */

 /**********************************************************************************************************************
 *  END OF FILE: ti_fee_cfg.h
 *********************************************************************************************************************/
