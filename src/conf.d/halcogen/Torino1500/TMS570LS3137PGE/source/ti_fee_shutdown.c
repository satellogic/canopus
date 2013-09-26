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
 *         File:  ti_fee_shutdown.c
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  None
 *
 *  Description:  This file implements the TI FEE Api TI_Fee_Shutdown.
 *---------------------------------------------------------------------------------------------------------------------
 * Author:  Vishwanath Reddy
 *---------------------------------------------------------------------------------------------------------------------
 * Revision History
 *---------------------------------------------------------------------------------------------------------------------
 * Version        Date         Author               Change ID        Description
 *---------------------------------------------------------------------------------------------------------------------
 * 00.01.00       31Aug2012    Vishwanath Reddy     0000000000000    Initial Version 
 * 00.01.01       29Oct2012    Vishwanath Reddy     0000000000000    Changes for implementing Error Recovery
 * 00.01.02       30Nov2012    Vishwanath Reddy     SDOCM00097786    Misra Fixes, Memory segmentation changes.
 * 00.01.05		  04Mar2013	   Vishwanath Reddy     SDOCM00099152    Added Deleting a block feature, bug fixes. 
 * 00.01.11       05Jul2013	   Vishwanath Reddy     SDOCM00101643	 Warning removal, changes done to write block 
                                                                     header.
 *
 *********************************************************************************************************************/

 /**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "ti_fee.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"

/***********************************************************************************************************************
 *  TI_Fee_Shutdown
 **********************************************************************************************************************/
/*! \brief      This function completes the Async jobs which are in progress 
 *				by performing a bulk data Write while shutting down the system synchronously.
  *  \param[in]	none
 *  \param[out] none
 *  \return     E_OK
 *  \return     E_NOT_OK
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
#if(TI_FEE_DRIVER == 1U)
Std_ReturnType TI_Fee_Shutdown(void)
{
	/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
	Std_ReturnType oResult=E_OK;
	uint8 u8EEPIndex = 0U;
	uint8 Fee_WriteCount=0U;
	uint16 u16BlockSize=0U;
	uint16 u16BlockNumber=0U;
	
	while(u8EEPIndex<TI_FEE_NUMBER_OF_EEPS)
	{
		if(TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync == 1U)
		{
			while(TRUE == TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteStartProgram)
			{
				if((FAPI_CHECK_FSM_READY_BUSY == Fapi_Status_FsmReady))
				{
					/* Mark the Block header to indicate start of programming to the Block*/
					TI_FeeInternal_StartProgramBlock(u8EEPIndex);
					/* Make the block status write flag as FALSE to indicate status write complete.*/
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteStartProgram = FALSE;
					/* Make the partial block status write flag as TRUE to indicate partial block write needs to happen.*/
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWritePartialBlockHeader	= TRUE;
				}
			}
			while(TRUE == TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWritePartialBlockHeader)
			{
				if((FAPI_CHECK_FSM_READY_BUSY == Fapi_Status_FsmReady))
				{
					/* Write Block number, Block Size and Block counter into Block Header */
					TI_FeeInternal_WriteBlockHeader((boolean)FALSE,u8EEPIndex,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSizeinBlockHeader,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockNumberinBlockHeader);
					/* Initialize Write */
					TI_FeeInternal_WriteInitialize(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress,TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8DataStart, u8EEPIndex);
					/* Make the partial block status write flag as FALSE to indicate status write complete.*/
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWritePartialBlockHeader	= FALSE;
				}
			}
			/* Write the remaining data */
			while(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize != 0U)
			{
				Fee_WriteCount = TI_FeeInternal_WriteDataF021(FALSE,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize,u8EEPIndex);
				if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize <= 0x8U)
				{	
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize = 0x0U;
				}	
				else
				{	
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-=Fee_WriteCount;							
				}				
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataWriteCount += Fee_WriteCount;
				/* Polling is required here because of bulk writing */
				/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason - Return value is not required."*/
				TI_FeeInternal_PollFlashStatus();
			}
			
			u16BlockSize = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSizeinBlockHeader;
			u16BlockNumber = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockNumberinBlockHeader;
			
			/* Mark Current Block Header as VALID */
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_oWriteAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader;						
			/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8Data=(uint8 *)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockHeader[0];
			
			/* Mark the Block as Valid */
			/*SAFETYMCUSW 28 D <REVIEWED> "Reason -  TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader is set to TRUE in TI_FeeInternal_WriteBlockHeader."*/
			/* After writing the block, mark the block as valid */
			while((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize==0U)||(TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteData==FALSE))
			{
				if((FAPI_CHECK_FSM_READY_BUSY == Fapi_Status_FsmReady))
				{
					/* Mark Current Block Header as VALID */
					if(0U == TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteCount)
					{
						/* First update CRC and Address of previous block */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oWriteAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader + 8U;
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8Data=(uint8 *)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockHeader[2];
						TI_FeeInternal_WriteBlockHeader((boolean)TRUE,u8EEPIndex,(uint16)u16BlockSize,(uint16)u16BlockNumber);
					}
					else
					{
						/* Update the block status as Valid */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oWriteAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader;
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8Data=(uint8 *)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockHeader[0];
						TI_FeeInternal_WriteBlockHeader((boolean)TRUE,u8EEPIndex,(uint16)u16BlockSize,(uint16)u16BlockNumber);
						break;
					}
				}
			}
			
			/* Update the status field of the previous Block header to INVALID */
			if(TI_Fee_GlobalVariables[u8EEPIndex].bWriteFirstTime!=TRUE)
			{
				/*SAFETYMCUSW 28 D <REVIEWED> "Reason -  TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader is set to TRUE in TI_FeeInternal_WritePreviousBlockHeader."*/
				while(TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader == TRUE)
				{
					if((FAPI_CHECK_FSM_READY_BUSY == Fapi_Status_FsmReady))
					{
						TI_FeeInternal_WritePreviousBlockHeader((boolean)TRUE,u8EEPIndex);
						/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason - Return value is not required."*/
						TI_FeeInternal_PollFlashStatus();
					}
				}								
			}		
			/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
			oResult = (uint8)E_OK;
			/* clear the Status word */
			TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync = 0U;
			if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult == JOB_PENDING)
			{
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_OK;
			}
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;
			#if(STD_OFF == TI_FEE_POLLING_MODE)
			TI_FEE_NVM_JOB_END_NOTIFICATION();
			#endif
			/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
			if(oResult == (uint8)E_NOT_OK)
			{
				if((TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.EraseImmediate == 0U) &&
				  (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.InvalidateBlock == 0U) &&
				  (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync == 0U)&&
				  (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read == 0U)&&
				  (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync == 0U)
				  )
				{
					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState != UNINIT)
					{
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;
					}
					else
					{
						/* For MISRA-C complaince */
					}
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
					TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync = 0U;
					#if(STD_OFF == TI_FEE_POLLING_MODE)
					TI_FEE_NVM_JOB_ERROR_NOTIFICATION();
					#endif
				}
			}
			else
			{
				/* For MISRA-C complaince */
			}
		}
		else if(((TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.EraseImmediate)== 1U)||((TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.InvalidateBlock) == 1U))
		{				
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteCount = 0U;
						
			/* Mark the status of current Block as VALID */	
			/*SAFETYMCUSW 28 D <REVIEWED> "Reason -  TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader is set to TRUE in TI_FeeInternal_WritePreviousBlockHeader."*/
			while(TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader==TRUE)
			{				
				TI_FeeInternal_WritePreviousBlockHeader(FALSE, u8EEPIndex);
				/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason - Return value is not required."*/
				TI_FeeInternal_PollFlashStatus();
			}		
		}
		else
		{
			/* For MISRA-C complaince */
		}
		u8EEPIndex++;	
	}
	return(oResult);
}
#endif

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 *  END OF FILE: ti_fee_shutdown.c
 *********************************************************************************************************************/
