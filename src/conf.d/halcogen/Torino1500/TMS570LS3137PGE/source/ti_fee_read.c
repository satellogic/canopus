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
 *         File:  ti_fee_read.c
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  None
 *
 *  Description:  This file implements the TI FEE Api TI_Fee_Read.
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
 * 00.01.03       14Jan2013    Vishwanath Reddy     SDOCM00098510    Changes as requested by Vector.
 * 00.01.04		  12Feb2012	   Vishwanath Reddy     SDOCM00099152    Integration issues fix. 
 * 00.01.05		  04Mar2013	   Vishwanath Reddy     SDOCM00099152    Added Deleting a block feature, bug fixes. 
 * 00.01.08		  05Apr2013	   Vishwanath Reddy     SDOCM00099152    Added feature : CRC check for unconfigured  blocks,
																	 Main function modified to complete writes as fast 
																	 as possible, Added Non polling mode support.
 * 00.01.09		  19Apr2013	   Vishwanath Reddy     SDOCM00099152    Warning removal, Added feature comparision of data  
																	 during write.																	 
 * 00.01.10       11Jun2013	   Vishwanath Reddy     SDOCM00101845	 Fixed issue with one EEPROM getting locked because 																 
                                                                     of error in second EEPROM.																	 
 *********************************************************************************************************************/

 /**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "ti_fee.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"
/***********************************************************************************************************************
 *  TI_Fee_Read
 **********************************************************************************************************************/
/*! \brief      This function performs the Read operation on the Block.It initializes the pointers. Complete read of the
 *				block will happen in TI_Fee_MainFunction.
 *  \param[in]  uint16 BlockNumber
 *  \param[in]	uint16 BlockOffset
 *  \param[in]	uint8* DataBufferPtr
 *  \param[in]	uint16 Length
 *  \param[out] none
 *  \return     E_OK
 *  \return     E_NOT_OK
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
Std_ReturnType TI_Fee_Read(uint16 BlockNumber,uint16 BlockOffset,uint8* DataBufferPtr,uint16 Length)
{
	uint32 u32BlockSize = 0U,au32BlockAddress[2],au32BlockStatus[2];
	TI_Fee_AddressType oBlockAddress=0U;
	uint16 u16BlockIndex=0U;	
	uint32 **ppu32ReadHeader=0U;	
	/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
	Std_ReturnType oResult=E_OK;
	uint16 u16DataSetNumber=0U;
	uint8 u8EEPIndex=0U;
	uint16 u16BlockNumber=0U;		
	TI_FeeModuleStatusType ModuleState=IDLE;		
	
	TI_Fee_u8DeviceIndex = 0U;		

	/* Determine the Block number & Block index */
	/* From the block number, remove data selection bits */
	u16BlockNumber = TI_FeeInternal_GetBlockNumber(BlockNumber);
	/* Get the index of the block in Fee_BlockConfiguration array */
	u16BlockIndex = TI_FeeInternal_GetBlockIndex(u16BlockNumber);	
	/* If block index is not found, report an error */
	if(u16BlockIndex == 0xFFFFU)
	{
	  TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus = TI_FEE_ERROR;
	  TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
	  TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error = Error_InvalidBlockIndex;
	}
	else
	{	
		/* Read the device index from the block configuration */
		u8EEPIndex = Fee_BlockConfiguration[u16BlockIndex].FeeEEPNumber;	
		ModuleState=TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState;
		if(UNINIT == ModuleState)
		{
			/* Module is Not Initialized */
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_FeeUninit;
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult=JOB_FAILED;
			/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
			oResult=E_NOT_OK;
		}
		else
		{
			TI_Fee_u8DeviceIndex = u8EEPIndex;
			/* Get the DataSet index after removing the Block number */
			/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason - Return value is used in following code."*/
			u16DataSetNumber = TI_FeeInternal_GetDataSetIndex(BlockNumber);
			/* Check for any non severe errors */		
			TI_FeeInternal_CheckForError(u8EEPIndex);
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus = TI_FEE_OK;		
		}
	}
	
	if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error == Error_Nil)
	{
		/* Store the module state */
		ModuleState = TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState;
		/* Read current state*/
		/* If the module state is BUSY_INTERNAL, change it to IDLE */
		oResult = TI_FeeInternal_CheckModuleState(u8EEPIndex);
		/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
		if((oResult == (uint8)E_OK) && (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read != 1U) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != JOB_PENDING))
		{		
			/* Initialize the DataSetIndex */
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex = 0U;			
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex = u16BlockIndex;
			/* Report error if Block index cannot be found */
			if(u16BlockIndex == 0xFFFFU)
			{
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
				/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
				oResult = E_NOT_OK;					
			}
			else
			{
				/* Determine the Block Size */
				u32BlockSize = Fee_BlockConfiguration[u16BlockIndex].FeeBlockSize;
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_PENDING;
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = BUSY;				
				/* Check if the parameters used for Fee_Read are proper or not */
				oResult = TI_FeeInternal_CheckReadParameters(u32BlockSize,BlockOffset,DataBufferPtr,Length,u8EEPIndex);
				/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
				if(oResult == (uint8)E_OK)
				{
					/* If using more than one DataSet, find out the DataSet Index. If Dataset Index is not found, report an error */
					if(Fee_BlockConfiguration[u16BlockIndex].FeeNumberOfDataSets > 1U)
					{
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex = u16DataSetNumber;
						if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex > Fee_BlockConfiguration[u16BlockIndex].FeeNumberOfDataSets)
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
							/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
							oResult = E_NOT_OK;								
						}						
					}

					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != JOB_FAILED)
					{
						/* Get the Current Block Address for this Block */
						oBlockAddress = TI_FeeInternal_GetCurrentBlockAddress(u16BlockIndex,u16DataSetNumber,u8EEPIndex);
						if(oBlockAddress == 0x00000000U)
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = BLOCK_INVALID;
							/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
							oResult = E_OK;								
						}	
					}

					if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != JOB_FAILED) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != BLOCK_INVALID))
					{
						/* Read the Block Header and check if it is valid */						
						/* If invalid, report an Error else continue reading the block */				
						au32BlockAddress[0] = oBlockAddress;						
						au32BlockAddress[1] = au32BlockAddress[0]+4U;
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
						ppu32ReadHeader = (uint32 **)&au32BlockAddress[0];							
						au32BlockStatus[0] = **ppu32ReadHeader;
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
						ppu32ReadHeader = (uint32 **)&au32BlockAddress[1];
						au32BlockStatus[1] = **ppu32ReadHeader;

						if((au32BlockStatus[0]==ValidBlockLo) && (au32BlockStatus[1] == ValidBlockHi))
						{	
							/* Read the Data */
							/* Determine the start address for the Read command */
							/* Start Address of data  = Current Address + Block Header Size + offset in the data */															
							/*SAFETYMCUSW 45 D MR:21.1 <REVIEWED> "Reason -  Null pointer check is done for oBlockAddress."*/
							/*SAFETYMCUSW 440 S MR:11.3 <REVIEWED> "Reason -  Casting is required here."*/
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8ReadAddress = (uint8 *)oBlockAddress ;								
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8ReadAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8ReadAddress+(uint32)TI_FEE_BLOCK_OVERHEAD+(uint32)BlockOffset;								
							/* If the length of the block to read is 0xffff, then the length of the block is read from the block header */
							if(Length == 0xFFFFU)
							{								
								/* Length of the block is present in 2 bytes of last 4 bytes*/		
								/* If block header is 24 bytes(0-23), 20-21 bytes are block size */
								oBlockAddress += (((TI_FEE_BLOCK_OVERHEAD >> 2U)-1U) << 2U);
								/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
								ppu32ReadHeader = (uint32 **)&oBlockAddress;
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize = (uint16)(((**ppu32ReadHeader)&0xFFFF0000U)>>16U);
								oBlockAddress -= (((TI_FEE_BLOCK_OVERHEAD >> 2U)-1U) << 2U);
							} 
							else
							{
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize = Length;
							}	
							/* Update the pointer. Reading will happen in main function */									
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8ReadDataBuffer = DataBufferPtr;									
							TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read = 1U;
							/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
							oResult = (uint8)E_OK;								
						}
						else
						{
							/* Invalid Block */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = BLOCK_INVALID;
							/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
							oResult = E_OK;								
						}																		
					}
				}
			}
			if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult == JOB_FAILED)	
			{
				TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read = 0U;
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;					
			}
		}
	}

	if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error!=Error_Nil)
	{
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
		/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
		oResult = E_NOT_OK;
	}
	/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
	if((oResult == (uint8)E_NOT_OK)||(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult == BLOCK_INVALID)||(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult == JOB_FAILED))
	{
		/* no other operations are in progress, change the module state if it is initialized */
		if((TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.EraseImmediate == 0U) &&
		   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.InvalidateBlock == 0U) &&
		   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync == 0U)&&
		   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync == 0U)&&
		   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read == 0U)
		  )
		{			
			/* Restore the module state */
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = ModuleState;
			TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read = 0U;	
			#if(STD_OFF == TI_FEE_POLLING_MODE)
			TI_FEE_NVM_JOB_ERROR_NOTIFICATION();				
			#endif
		}
	}
	#if(TI_FEE_NUMBER_OF_EEPS==2U)
	TI_Fee_oStatusWord_Global.Fee_u16StatusWord = ((TI_Fee_oStatusWord[0].Fee_u16StatusWord) |
												   (TI_Fee_oStatusWord[1].Fee_u16StatusWord));
	#endif
	
    return(oResult);
}
#define FEE_STOP_SEC_CODE
#include "MemMap.h"
/**********************************************************************************************************************
 *  END OF FILE: ti_fee_read.c
 *********************************************************************************************************************/
