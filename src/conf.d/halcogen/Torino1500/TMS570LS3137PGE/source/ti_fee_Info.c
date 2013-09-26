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
 *         File:  ti_fee_info.c
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  None
 *
 *  Description:  This file implements the TI FEE Misc Api's.
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
 *********************************************************************************************************************/

 
 /**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "ti_fee.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"

/***********************************************************************************************************************
 *  TI_Fee_GetVersionInfo
 **********************************************************************************************************************/
/*! \brief      This function returns the version information for the TI Fee module.
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \return     none
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
void TI_Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr) 
{
	if(VersionInfoPtr != 0U)
	{
		VersionInfoPtr->sw_major_version = TI_FEE_MAJOR_VERSION;
		VersionInfoPtr->sw_minor_version = TI_FEE_MINOR_VERSION;
		VersionInfoPtr->sw_patch_version = TI_FEE_PATCH_VERSION;
	}	
}

/***********************************************************************************************************************
 *  TI_Fee_GetJobResult
 **********************************************************************************************************************/
/*! \brief      This function returns the Job result.
 *  \param[in]  u8EEPIndex
 *  \param[out] none
 *  \return     Job Result
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
TI_FeeJobResultType TI_Fee_GetJobResult(uint8 u8EEPIndex)
{
	return(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult);
}

/***********************************************************************************************************************
 *  TI_FeeErrorCode
 **********************************************************************************************************************/
/*! \brief      This function returns the Error status of the TI Fee module.
 *  \param[in]  u8EEPIndex
 *  \param[out] none
 *  \return     Error Code 
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
TI_Fee_ErrorCodeType TI_FeeErrorCode(uint8 u8EEPIndex)
{
	return(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error);
}

#define FEE_STOP_SEC_CODE
#include "MemMap.h"
/**********************************************************************************************************************
 *  END OF FILE: ti_fee_info.c
 *********************************************************************************************************************/
