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
 *         File:  ti_fee_writeSync.c
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  None
 *
 *  Description:  This file implements the TI FEE Api, TI_Fee_WriteSync.
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
 * 00.01.11       05Jul2013	   Vishwanath Reddy     SDOCM00101643	 Warning removal, Changes done to job result and 
                                                                     module state when copy is initiated.
 *********************************************************************************************************************/

 /**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "ti_fee.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"

/***********************************************************************************************************************
 *  TI_Fee_WriteAsync
 **********************************************************************************************************************/
/*! \brief      This function initiates the Synchronous Write operation to a Block
 *  \param[in]  uint16 BlockNumber
 *  \param[in] 	uint8* DataBufferPtr
 *  \param[out] none
 *  \return     E_OK
 *  \return     E_NOT_OK
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
#if(TI_FEE_DRIVER == 1U)
Std_ReturnType TI_Fee_WriteSync(uint16 BlockNumber,uint8* DataBufferPtr)
{	
	/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
	Std_ReturnType oResult = E_OK;
	uint16 u16BlockNumber = 0U;	
	uint16 u16BlockSize = 0U;	
	uint16 u16DataSetNumber = 0U;
	uint16 u16BlockIndex = 0U;
	uint8 u8EEPIndex = 0U;
	uint8 u8WriteCount = 0U;
	boolean bDoNotWrite = FALSE;
	#if(TI_FEE_FLASH_CRC_ENABLE == STD_ON)
	uint32 u32CheckSum = 0U;
	#else
	uint8 *pu8ReadData;
	uint8 *pu8ReadSourceData;
	uint16 u16LoopIndex;
	#endif	
	boolean bError=FALSE;
	#if((TI_FEE_FLASH_WRITECOUNTER_SAVE == STD_ON) || (TI_FEE_FLASH_CRC_ENABLE == STD_ON))
	uint32 **ppu32ReadHeader = 0U;
	#endif		
	TI_FeeModuleStatusType ModuleState=IDLE;	
		
	TI_Fee_u8DeviceIndex = 0U;
	
	/* Check if the DataBufferPtr is a Null pointer */
	if(DataBufferPtr == 0U)
	{				
		bError = TRUE;
		/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
		oResult = (uint8)E_NOT_OK;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_NullDataPtr;
		#if(TI_FEE_NUMBER_OF_EEPS==2U)
		TI_Fee_GlobalVariables[u8EEPIndex+1U].Fee_oStatus=TI_FEE_ERROR;
		TI_Fee_GlobalVariables[u8EEPIndex+1U].Fee_Error=Error_NullDataPtr;
		TI_Fee_GlobalVariables[u8EEPIndex+1U].Fee_u16JobResult=JOB_FAILED;
		#endif
	}	
	/*SAFETYMCUSW 139 S MR:13.7 <REVIEWED> "Reason - bError is set to TRUE in above lines."*/	
	if(FALSE == bError)
	{
		/* Determine the Block number & Block index */
		/* From the block number, remove data selection bits */
		u16BlockNumber = TI_FeeInternal_GetBlockNumber(BlockNumber);
		/* Get the index of the block in Fee_BlockConfiguration array */
		u16BlockIndex = TI_FeeInternal_GetBlockIndex(u16BlockNumber);
		/* If block index is not found, report an error */
		if(u16BlockIndex == 0xFFFFU)
		{
		  TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus = TI_FEE_ERROR;
		  TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error = Error_InvalidBlockIndex;
		  #if(TI_FEE_NUMBER_OF_EEPS==2U)
		  TI_Fee_GlobalVariables[u8EEPIndex+1U].Fee_oStatus=TI_FEE_ERROR;
		  TI_Fee_GlobalVariables[u8EEPIndex+1U].Fee_Error=Error_InvalidBlockIndex;
		  #endif
		}
		else
		{	
			/* Read the device index from the block configuration */
			u8EEPIndex  =  Fee_BlockConfiguration[u16BlockIndex].FeeEEPNumber;	
			/* Check the module state */
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
				u16DataSetNumber  =  TI_FeeInternal_GetDataSetIndex(BlockNumber);
				/* Check for any non severe errors */		
				TI_FeeInternal_CheckForError(u8EEPIndex);	
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus = TI_FEE_OK;				
			}
		}
		if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error == Error_Nil)
		{
			/* Store the module state */
			ModuleState = TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState;

			/* If the module state is BUSY_INTERNAL, change it to IDLE */
			oResult = TI_FeeInternal_CheckModuleState(u8EEPIndex);
			/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
			if((oResult == (uint8)E_OK) && (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync != 1U) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != JOB_PENDING))
			{			
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = BUSY;
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_PENDING;
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataWriteCount = 0U;				
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex = 0U;					
				/* Update Block Index in to array */				
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex = u16BlockIndex;
				/* Update the data set number */
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex = u16DataSetNumber;
				/* Report an error if Block Index is not found */
				if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex == 0xFFFFU)
				{
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
					/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
					oResult = (uint8)E_NOT_OK;
				}
				else
				{
					/* Get the Address for the Current Block */
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress = TI_FeeInternal_GetCurrentBlockAddress(u16BlockIndex,u16DataSetNumber,u8EEPIndex);
					
					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress == 0x00000000U)
					{
						/* Writing block for first time */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error = Error_CurrentAddress;
						TI_Fee_GlobalVariables[u8EEPIndex].bWriteFirstTime = TRUE;
					}
					else
					{
						/* Block already existing */
						#if(TI_FEE_FLASH_WRITECOUNTER_SAVE == STD_ON)
						/* Get the block erase count */
						/* If block header is 24 bytes(0-23), 16-19 bytes are erase count */						
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress+=(((TI_FEE_BLOCK_OVERHEAD >> 2U)-2U) << 2U);
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/
						ppu32ReadHeader=(uint32 **)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress;
						TI_Fee_u32BlockEraseCount  =  **ppu32ReadHeader;
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress-=(((TI_FEE_BLOCK_OVERHEAD >> 2U)-2U) << 2U);
						TI_Fee_u32BlockEraseCount++;
						#endif
					}
					
					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult != JOB_FAILED)
					{							
						/* Get the Block Size. Function will return Block size+BlockHeader size */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize = TI_FeeInternal_GetBlockSize((uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex);					
						/*Store block size for writing into block header */
						u16BlockSize = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-TI_FEE_BLOCK_OVERHEAD;
						
						/*If CRC check if enabled, perform CRC on the data to be written and check if CRC matches in EEP */
						#if(TI_FEE_FLASH_CRC_ENABLE == STD_ON)
						TI_Fee_u32FletcherChecksum = TI_FeeInternal_Fletcher16(DataBufferPtr, TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-TI_FEE_BLOCK_OVERHEAD);
						TI_Fee_u32FletcherChecksum |= (0xFFFF0000U) ;
						if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_CurrentAddress)
						{							
							/* Read the CRC */
							/* If block header is 24 bytes(0-23), 12-15 bytes are CRC */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress+=(((TI_FEE_BLOCK_OVERHEAD >> 2U)-3U) << 2U);
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/
							ppu32ReadHeader = (uint32 **)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress;
							u32CheckSum = **ppu32ReadHeader;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress -= (((TI_FEE_BLOCK_OVERHEAD >> 2U)-3U) << 2U);
							if(TI_Fee_u32FletcherChecksum == u32CheckSum)
							{
								bDoNotWrite = TRUE;
								/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
								oResult = (uint8)E_OK;
								/* Restore the status since no writing will happen */
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_OK;
								#if(STD_OFF == TI_FEE_POLLING_MODE)
								TI_FEE_NVM_JOB_END_NOTIFICATION();	
								#endif
							}
						}
						else if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error == Error_CurrentAddress)
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error = Error_Nil;
						}
						#else						
						pu8ReadData = (uint8 *)(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress + TI_FEE_BLOCK_OVERHEAD);
						pu8ReadSourceData = DataBufferPtr;
						bDoNotWrite = TRUE;
						for(u16LoopIndex = 0; u16LoopIndex < TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-TI_FEE_BLOCK_OVERHEAD; u16LoopIndex++)
						{
							if(*pu8ReadData != *pu8ReadSourceData)
							{
								bDoNotWrite = FALSE;
								break;
							}
							else
							{
								pu8ReadData++;
								pu8ReadSourceData++;
							}							
						}
						if(TRUE == bDoNotWrite)	
						{
							/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
							oResult = (uint8)E_OK;
							/* Restore the status since no writing will happen */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_OK;
							#if(STD_OFF == TI_FEE_POLLING_MODE)
							TI_FEE_NVM_JOB_END_NOTIFICATION();	
							#endif
						}	
						#endif	
						if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error == Error_CurrentAddress)
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error = Error_Nil;
						}
						/* If using more than one DataSet, find out the DataSet Index. If Dataset Index is not found, report an error */
						if(Fee_BlockConfiguration[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex].FeeNumberOfDataSets>1U)
						{							
							if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex >= Fee_BlockConfiguration[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex].FeeNumberOfDataSets)
							{
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
								/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
								oResult = (uint8)E_NOT_OK;
							}						
						}											
					}
					if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult!=JOB_FAILED) && (bDoNotWrite == FALSE))
					{						
						/* Get the Next Flash Address for this Block */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress = TI_FeeInternal_GetNextFlashAddress(u8EEPIndex,u16BlockIndex,u16DataSetNumber);
						/* Check if the memory space to write this block is free */
						TI_FeeInternal_SanityCheck((uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize,u8EEPIndex);						
						/* If the Virtual Sector size is exceeded by this Write operation */
						/* Find out the next Virtual Sector to write to, mark that as Copy */
						if(((TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress+TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize)>TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress))
						{
							/* Find the next Virtual Sector to write to */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector = TI_FeeInternal_FindNextVirtualSector(u8EEPIndex);

							/* Configure it as a Copy Virtual Sector */
							if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U)&&(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error == Error_Nil))
							{
								/* Immediately Update the VS state to COPY */
								TI_FeeInternal_WriteVirtualSectorHeader((uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector,VsState_Copy, u8EEPIndex);																								
							}							
							else
							{
								/* If no Virtual Sector found, return an Error */
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
								/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
								oResult = (uint8)E_NOT_OK;
							}
							if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult!=JOB_FAILED)
							{
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCopyVirtualSectorAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress;
								/* Next data write happens after VS Header */
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextCopyVSwriteaddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress+TI_FEE_VIRTUAL_SECTOR_OVERHEAD+16U;
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress  =  TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextCopyVSwriteaddress;
								/* Clear the block copy status of all blocks. All blocks need to be copied in background */
								TI_FeeInternal_SetClearCopyBlockState(u8EEPIndex,FALSE);
								/* update the block copy status for current block to copied. */
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8BlockCopyStatus[u16BlockIndex][u16DataSetNumber]=1U;
								/* Update Next write address */
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextCopyVSwriteaddress;									
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress = TI_FeeInternal_AlignAddressForECC(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress);										
								/* Update the status to COPY. This will enable copying of all blocks from Active VS to Copy VS in background*/
								TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Copy = 1U;									
							}
						}						
					}
					if(((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult!=JOB_FAILED)||(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus!=TI_FEE_ERROR)) && (bDoNotWrite == FALSE))
					{
						if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress>=TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress)				   			
						{						 					  
						  /* Update the next free write address */			
						  TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress+TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize;						  
						  TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress = TI_FeeInternal_AlignAddressForECC(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress);													  
						  
						  /* Update the next free write address in COPY or ACTIVE VS */
						  if(TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Copy == 1U)
						  {
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextCopyVSwriteaddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress;
						  }
						  else
						  {
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextActiveVSwriteaddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress;
						  }						 
						}
						else
						{
						
						}						
						
						/* update address for current block. This will be used in Fee_WriteBlockHeader for writing block header */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress;
						/* Initialize the write address for block header */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oWriteAddress  =  TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress;	
						/* Mark the Block header to indicate start of programming to the Block*/
						TI_FeeInternal_StartProgramBlock(u8EEPIndex);
						TI_FeeInternal_PollFlashStatus();
						/* Write Block number, Block Size and Block counter into Block Header */
						TI_FeeInternal_WriteBlockHeader((boolean)FALSE,u8EEPIndex,(uint16)u16BlockSize,(uint16)BlockNumber);
						/* Update block Address array */							
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockOffset[u16BlockIndex][u16DataSetNumber]=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress ;
						/* Update the block copy status */						
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8BlockCopyStatus[u16BlockIndex][u16DataSetNumber] = 1U;
						/* Initialize Write */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-=TI_FEE_BLOCK_OVERHEAD;
						TI_FeeInternal_WriteInitialize(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oFlashNextAddress,DataBufferPtr, u8EEPIndex);
						/* Write Job in Progress */
						TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync = 1U;						
						
						/* Write the entire data for the Block */
						while(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize!=0U)
						{
							u8WriteCount = TI_FeeInternal_WriteDataF021(FALSE,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize, u8EEPIndex);				
							/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason - Return value is not required."*/
							TI_FeeInternal_PollFlashStatus();
							if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize <= 0x8U)
							{	
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize = 0x0U;
							}	
							else
							{	
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize-=u8WriteCount;							
							}								
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataWriteCount+=u8WriteCount;														
						}
						
						/* Mark Current Block Header as VALID */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oWriteAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader;						
						/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_pu8Data=(uint8 *)&TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockHeader[0];
						/*SAFETYMCUSW 28 D <REVIEWED> "Reason -  TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader is set to TRUE in TI_FeeInternal_WriteBlockHeader."*/
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
						TI_Fee_GlobalVariables[u8EEPIndex].bWriteFirstTime = FALSE;	
						/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
						oResult = (uint8)E_OK;
						/* Clear the status word */
						TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync = 0U;
						if(TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Copy == 1U)
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_PENDING;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = BUSY_INTERNAL;
						}
						else
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_OK;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = IDLE;
							#if(STD_OFF == TI_FEE_POLLING_MODE)
							TI_FEE_NVM_JOB_END_NOTIFICATION();
							#endif
						}												
					}
				}				
			}
		}

		if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error!=Error_Nil)
		{
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
			/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/
			oResult = (uint8)E_NOT_OK;
		}
		/*SAFETYMCUSW 331 S MR:10.1 <REVIEWED> "Reason - Std_ReturnType is not part of FEE.This should be fixed outside of FEE."*/	
		if((oResult == (uint8)E_NOT_OK)||(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult == JOB_FAILED))
		{
			/* If any other operation is in progress, don't change the module state */
			if((TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.EraseImmediate == 0U) &&
			   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.InvalidateBlock == 0U) &&
			   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteAsync == 0U)&&
			   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync == 0U)&&
			   (TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Read == 0U)
			  )		  
			{			
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_FAILED;
				TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.WriteSync = 0U;
				#if(STD_OFF == TI_FEE_POLLING_MODE)
				TI_FEE_NVM_JOB_ERROR_NOTIFICATION();
				#endif
				/* Restore the module state */
				if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_InvalidBlockIndex)
				{
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = ModuleState;
				}			
			}
		}
		#if(TI_FEE_NUMBER_OF_EEPS==2U)
		TI_Fee_oStatusWord_Global.Fee_u16StatusWord = ((TI_Fee_oStatusWord[0].Fee_u16StatusWord) |
													   (TI_Fee_oStatusWord[1].Fee_u16StatusWord));
		#endif
  }
  return(oResult);
}
#endif

#define FEE_STOP_SEC_CODE
#include "MemMap.h"
/**********************************************************************************************************************
 *  END OF FILE: ti_fee_writeSync.c
 *********************************************************************************************************************/
