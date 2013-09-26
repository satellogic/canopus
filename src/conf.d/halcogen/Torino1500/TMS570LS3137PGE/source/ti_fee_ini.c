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
 *         File:  ti_fee_ini.c
 *      Project:  Tms570_TIFEEDriver
 *       Module:  TIFEEDriver
 *    Generator:  None
 *
 *  Description:  This file implements the TI FEE Api TI_Fee_Init.
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
 * 00.01.06		  11Mar2013	   Vishwanath Reddy     SDOCM00099152    Added feature : copying of unconfigured blocks. 
 * 00.01.07		  15Mar2013	   Vishwanath Reddy     SDOCM00099152    Added feature : Number of 8 bytes writes, fixed 
																	 issue with copy blocks. 
 * 00.01.08		  05Apr2013	   Vishwanath Reddy     SDOCM00099152    Added feature : CRC check for unconfigured  blocks,
																	 Main function modified to complete writes as fast 
																	 as possible, Added Non polling mode support.																	 
 * 00.01.09		  19Apr2013	   Vishwanath Reddy     SDOCM00099152    Warning removal, Added feature comparision of data  
																	 during write.		
 * 00.01.10       11Jun2013	   Vishwanath Reddy     SDOCM00101845	 Updated version info.
                                                                     Enabled SECDED.                
                                                                     Updated logic to know if erase was started in 
                                                                     previous cycle.
                                                                     Moved block writing by 16 bytes after VS Header.   
 * 00.01.11       05Jul2013	   Vishwanath Reddy     SDOCM00101643	 Updated version information.																	 																	 
 *                
 *********************************************************************************************************************/

 
 /**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "ti_fee.h"

/**********************************************************************************************************************
 *  VERSION CHECK
 *********************************************************************************************************************/
#if (TI_FEE_MAJOR_VERSION != 3U)
    #error TI_FEE_Cfg.c: TI_FEE_MAJOR_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_MAJOR_VERSION */
#if (TI_FEE_MINOR_VERSION != 0U)
    #error TI_FEE_Cfg.c: TI_FEE_MINOR_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_MINOR_VERSION */
#if (TI_FEE_PATCH_VERSION != 2U)
    #error TI_FEE_Cfg.c: TI_FEE_PATCH_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_PATCH_VERSION */
#if (TI_FEE_SW_MAJOR_VERSION != 0U)
    #error TI_FEE_Cfg.c: TI_FEE_SW_MAJOR_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_MAJOR_VERSION */
#if (TI_FEE_SW_MINOR_VERSION != 1U)
    #error TI_FEE_Cfg.c: TI_FEE_SW_MINOR_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_MINOR_VERSION */
#if (TI_FEE_SW_PATCH_VERSION != 11U)
    #error TI_FEE_Cfg.c: TI_FEE_SW_PATCH_VERSION of TI_FEE.h is incompatible.
#endif /* FEE_SW_PATCH_VERSION */

/**********************************************************************************************************************
 *  Configuration CHECK
 *********************************************************************************************************************/
#if ((TI_FEE_NUMBER_OF_EEPS > 2U))
	#error ti_fee_ini.c: Number of EEP's can't be more than 2.Check configuration file.
#endif	
#if ((TI_FEE_DATASELECT_BITS > 8U))
	#error ti_fee_ini.c: Data selection bits cannot be more than 8.Check configuration file.
#endif	
	

/**********************************************************************************************************************
 *  Global Definitions
 *********************************************************************************************************************/
#define FEE_START_SEC_VAR_INIT_UNSPECIFIED  
#include "MemMap.h"

TI_Fee_GlobalVarsType TI_Fee_GlobalVariables[TI_FEE_NUMBER_OF_EEPS];
uint16 TI_Fee_u16DataSets;
uint32 TI_Fee_u32FletcherChecksum;
uint32 TI_Fee_u32BlockEraseCount;
uint8  TI_Fee_u8DeviceIndex;
uint16 TI_Fee_u16ActCpyVS;
TI_Fee_StatusWordType_UN TI_Fee_oStatusWord[TI_FEE_NUMBER_OF_EEPS];
Fapi_FlashStatusWordType TI_FlashStatusWord,*poTI_FlashStatusWord=&TI_FlashStatusWord;
#if (TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY != 0U)
uint16 TI_Fee_u16NumberOfUnconfiguredBlocks[TI_FEE_NUMBER_OF_EEPS];
#endif
#if(TI_FEE_NUMBER_OF_EEPS==2U)
TI_Fee_StatusWordType_UN TI_Fee_oStatusWord_Global;
#endif

#define FEE_STOP_SEC_VAR_INIT_UNSPECIFIED  
#include "MemMap.h"

#define FEE_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"
/*  General Global Variables/defines */
const TI_Fee_GeneralConfigType TI_Fee_General=
{
  	TI_FEE_INDEX,
  	TI_FEE_FLASH_ERROR_CORRECTION_ENABLE,
	TI_FEE_VIRTUAL_PAGE_SIZE,  	  	
  	TI_FEE_OPERATING_FREQUENCY
};

/* Defining the Published information */
const TI_Fee_PublishedInformationType TI_Fee_PublishedInformation =
{
    TI_FEE_BLOCK_OVERHEAD,
    TI_FEE_MAXIMUM_BLOCKING_TIME,
    TI_FEE_PAGE_OVERHEAD,
    TI_FEE_VIRTUAL_SECTOR_OVERHEAD
};
#define FEE_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"

/***********************************************************************************************************************
 *  TI_Fee_Init
 **********************************************************************************************************************/
/*! \brief      This function is used to initialize the TI Fee module.
 *				It determines which Virtual Sector to use, sets up the Flash state machine.
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \return     none
 *  \context    Function could be called from task level
 *  \note       TI FEE API.
 **********************************************************************************************************************/
void TI_Fee_Init(void)
{
	uint32 au32VirtualSectorHeader[6];
	uint32 au32VirtualSectorAddress[6];
	uint32 u32VirtualSectorStartAddress = 0U;
	uint16 u16Index=0U;
	uint16 u16Index1=0U;
	Fapi_FlashSectorType oSectorStart,oSectorEnd;
	uint32 **ppu32ReadHeader = 0U;	
	TI_Fee_AddressType oCopyVirtualSectorStartAddress = 0U;
	uint8 u8EEPIndex=0U;
	boolean bActiveVSScanned[TI_FEE_NUMBER_OF_EEPS];
	boolean bFoundActiveVS[TI_FEE_NUMBER_OF_EEPS];
	boolean bFoundCopyVS[TI_FEE_NUMBER_OF_EEPS];	
	boolean bFoundReadyForEraseVS[TI_FEE_NUMBER_OF_EEPS];
	boolean	bFoundActiveVirtualSector[TI_FEE_NUMBER_OF_EEPS];
	boolean bFoundReadyforEraseVirtualSector[TI_FEE_NUMBER_OF_EEPS];
	TI_Fee_u32BlockEraseCount = 0xFFFFFFFF;
	
	while(u8EEPIndex<TI_FEE_NUMBER_OF_EEPS)
	{	
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress=0xFFFFFFFFU;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockIndex=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockCopyIndex=0xFFFFU;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetIndex=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16BlockSize=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetBits=0U;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState=UNINIT;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentAddress=0U;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentBlockHeader=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress=0U;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bInvalidWriteBit=FALSE;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteData=FALSE;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteCount=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteCopyVSHeader=0U;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteBlockHeader=FALSE;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyBlockNumber=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_Nil;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_OK;				
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16InternalEraseQueue=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataWriteCount=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8InternalVirtualSectorEnd=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8InternalVirtualSectorStart=0U;
		TI_Fee_GlobalVariables[u8EEPIndex].bWriteFirstTime=FALSE;
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bFindNextVirtualSector=FALSE;		
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteVSHeader = FALSE;	
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWriteStartProgram = FALSE;	
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_bWritePartialBlockHeader	= FALSE;	
		TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCurrentStartAddress = 0xFFFFFFFFU;
		
		bActiveVSScanned[u8EEPIndex] = FALSE;
		bFoundActiveVS[u8EEPIndex] = FALSE;
		bFoundCopyVS[u8EEPIndex] = FALSE;
		bFoundReadyForEraseVS[u8EEPIndex] = FALSE;
		bFoundActiveVirtualSector[u8EEPIndex] = FALSE;
		bFoundReadyforEraseVirtualSector[u8EEPIndex] = FALSE;
		
		if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus==TI_FEE_OK)&& (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error==Error_Nil))
		{
			/* Determine how many DataSets are being used */
			TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetBits = (1U<<TI_FEE_DATASELECT_BITS)-1U;			
			/* Update total number of data sets in to global variable. If TI_FEE_DATASELECT_BITS=3, total data sets per block would be 2 power 3 = 8 */
			TI_Fee_u16DataSets = TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16DataSetBits;								

			if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error==Error_Nil)
			{
				/* Determine the Write Size supported by the device.*/				
				/*SAFETYMCUSW 93 S MR:6.1,6.1,10.1,10.2,10.3,10.4 <REVIEWED> "Reason -  WIDTH_EEPROM_BANK is defined in F012 files.Hence it cannot be corrected here."*/
				/*SAFETYMCUSW 184 S LDRA adding spaces causes this rule to fail."*/	
				TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteSize = (uint8)WIDTH_EEPROM_BANK;				

				/* Initialize the Status word */
				TI_Fee_oStatusWord[u8EEPIndex].Fee_u16StatusWord=0U;
				
				/* Initialize the copy status of all the Blocks */
				/* Initialize the block offset address of all the Blocks */
				for(u16Index=0U;u16Index<TI_FEE_NUMBER_OF_BLOCKS;u16Index++)	
				{
					for(u16Index1=0U;u16Index1<=TI_Fee_u16DataSets;u16Index1++)	
					{
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8BlockCopyStatus[u16Index][u16Index1]=2U;
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32BlockOffset[u16Index][u16Index1]= 0x0BAD0BADU;											
					}				
				}
				#if (TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY != 0U)
				for(u16Index=0U;u16Index<TI_FEE_NUMBER_OF_UNCONFIGUREDBLOCKSTOCOPY;u16Index++)	
				{					
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32UnConfiguredBlockAddress[u16Index]= 0x0BAD0BADU;
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8UnConfiguredBlockCopyStatus[u16Index]= 2U;																
				}
				#endif
				/* Find the Active Virtual Sector */
				/* Read the virtual sector header and find if it matches 0x000000000000FFFF for active */
				/* If Active, calculate the next flash address */
				/* If not, parse the next Virutal Sector */
				/* Else create a new active Virtual Sector header */
				if(0U == u8EEPIndex)
				{
					u16Index=0U;
					u16Index1=TI_FEE_NUMBER_OF_VIRTUAL_SECTORS - TI_FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1;	
				}
				else
				{
					u16Index = TI_FEE_NUMBER_OF_VIRTUAL_SECTORS_EEP1;
					u16Index1 = TI_FEE_NUMBER_OF_VIRTUAL_SECTORS;
				}	
				while(u16Index < u16Index1)				
				{
					/* Initialize the state of all the Virtual Sectors */	
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=0U;	
					
					u32VirtualSectorStartAddress=0xFFFFFFFFU;									
					oSectorStart=Fee_VirtualSectorConfiguration[u16Index].FeeStartSector;										
					/* Get the starting address of the VS */
					/*SAFETYMCUSW 91 D MR:16.10 <REVIEWED> "Reason -  Return value is used in next lines if there is no error."*/
					u32VirtualSectorStartAddress=TI_FeeInternal_GetVirtualSectorParameter(oSectorStart,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank,(boolean)TRUE,u8EEPIndex);                          

					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error==Error_Nil)
					{						
						if((u32VirtualSectorStartAddress)!=0xFFFFFFFFU)
						{
							/* Read this address and find whether it is Active or not */
							au32VirtualSectorAddress[0]=u32VirtualSectorStartAddress;
							au32VirtualSectorAddress[1]=u32VirtualSectorStartAddress+4U;
							au32VirtualSectorAddress[2]=u32VirtualSectorStartAddress+8U;
							au32VirtualSectorAddress[3]=u32VirtualSectorStartAddress+12U;
							au32VirtualSectorAddress[4]=u32VirtualSectorStartAddress+16U;
							au32VirtualSectorAddress[5]=u32VirtualSectorStartAddress+20U;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[0];
							au32VirtualSectorHeader[0] = **ppu32ReadHeader;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[1];
							au32VirtualSectorHeader[1] = **ppu32ReadHeader;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[2];
							au32VirtualSectorHeader[2] = **ppu32ReadHeader;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[3];
							au32VirtualSectorHeader[3] = **ppu32ReadHeader;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[4];
							au32VirtualSectorHeader[4] = **ppu32ReadHeader;
							/*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <REVIEWED> "Reason -  Casting is required here."*/									 
							ppu32ReadHeader=(uint32 **)&au32VirtualSectorAddress[5];
							au32VirtualSectorHeader[5] = **ppu32ReadHeader;
							
							/* Get the erase count of the VS */							
							if((au32VirtualSectorHeader[3] & 0x00FFFFF0U)!=0x00FFFFF0U)
							{							
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32VirtualSectorEraseCount[u16Index]=((au32VirtualSectorHeader[3] & 0x00FFFFF0U) >> 0x4U) ;						
							}
							else
							{
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au32VirtualSectorEraseCount[u16Index]=0U;
							}		
							
							/* Check if the sector is active VS */
							if(((au32VirtualSectorHeader[0]==ActiveVSLo) && (au32VirtualSectorHeader[1]==ActiveVSHi)) ||
							   (TRUE == bFoundActiveVS[u8EEPIndex])
							  )	
							{								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_Active;								
								
								if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector!=0U)
								{
									/* Two or more Active Virtual sectors found */
									/* Report Error */
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_TwoActiveVS;
									/* Update which of the VS's are Active */
									TI_Fee_u16ActCpyVS = 0x1U << (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector-1U);
									TI_Fee_u16ActCpyVS |= 0x1U << u16Index;
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
									bFoundActiveVirtualSector[u8EEPIndex]=FALSE;
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector=0U;
								}
								else
								{
									/* Active virtual sector. Get the VS index */									
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector=Fee_VirtualSectorConfiguration[u16Index].FeeVirtualSectorNumber; 									
									bFoundActiveVirtualSector[u8EEPIndex]=TRUE;
									bFoundActiveVS[u8EEPIndex] = FALSE;
									if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector==0U)
									{
										/* Determine the Start & End address of the Active virtual sector */
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress=u32VirtualSectorStartAddress;									
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorStartAddress=u32VirtualSectorStartAddress;										
										oSectorEnd=Fee_VirtualSectorConfiguration[u16Index].FeeEndSector; 																			
										/* Get the start address of the end VS */
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress=TI_FeeInternal_GetVirtualSectorParameter(oSectorEnd,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank,(boolean)TRUE, u8EEPIndex); 
										if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress==0xFFFFFFFFU)
										{
											TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
										}
										/* Get the length of VS and add to start address of the end VS. Now we have VS end address */
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress+=TI_FeeInternal_GetVirtualSectorParameter(oSectorEnd,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank,FALSE, u8EEPIndex); 
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorEndAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress;
										if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error!=Error_Nil)
										{
											TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
										}
									}
									/* Memorize the start address of VS as start address of active VS */
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress=u32VirtualSectorStartAddress;
								}
							}							
							else if((au32VirtualSectorHeader[0]==CopyVSLo) && (au32VirtualSectorHeader[1]==CopyVSHi) ||
							        (TRUE == bFoundCopyVS[u8EEPIndex])
							        )
							{	
								/* Check if the sector is copy VS */								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_Copy;								
										
								if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U)
								{
									/* Two or more Copy Virtual Sectors found */
									/* Report Error */
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_TwoCopyVS;
									/* Update which of the VS's are Copy */
									TI_Fee_u16ActCpyVS = 0x1U << (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector-1U);
									TI_Fee_u16ActCpyVS |= 0x1U << u16Index;
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
								}
								else
								{
									/* Copy Virtual Sector found */
									/* Determine the Start & End address of the Copy Virtual Sector */									
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector=Fee_VirtualSectorConfiguration[u16Index].FeeVirtualSectorNumber;										
									bFoundCopyVS[u8EEPIndex] = FALSE;								
									oCopyVirtualSectorStartAddress=u32VirtualSectorStartAddress;	
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCopyVirtualSectorStartAddress=u32VirtualSectorStartAddress;																	
									oSectorEnd=Fee_VirtualSectorConfiguration[u16Index].FeeEndSector;									
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress=TI_FeeInternal_GetVirtualSectorParameter(oSectorEnd,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank,(boolean)TRUE, u8EEPIndex);
									if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress==0xFFFFFFFFU)
									{
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
									}									
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress+=TI_FeeInternal_GetVirtualSectorParameter(oSectorEnd,(uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank,FALSE, u8EEPIndex);
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCopyVirtualSectorEndAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorEndAddress;
									if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error!=Error_Nil)
									{
										TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
									}									
								}
							}							
							else if((au32VirtualSectorHeader[0]==EmptyVSLo) && (au32VirtualSectorHeader[1]==EmptyVSHi))
							{
								/* Check if the sector is Empty VS */								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_Empty;									
							}							
							else if((au32VirtualSectorHeader[0]==InvalidVSLo) && (au32VirtualSectorHeader[1]==InvalidVSHi))
							{
								/* Check if the sector is Invalid VS */
								/* If it is invalid, add to internal erase queue. Erasing will happen in background */																
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_Invalid;								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16InternalEraseQueue|=0x1U<<u16Index;	
							}							
							else if((au32VirtualSectorHeader[0]==ReadyforEraseVSLo) && (au32VirtualSectorHeader[1]==ReadyforEraseVSHi)
									|| (TRUE == bFoundReadyForEraseVS[u8EEPIndex]))
							{
								/* Check if the sector is Ready for Erase VS */
								/* If it is ready for erase, add to internal erase queue. Erasing will happen in background */								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_ReadyForErase;
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16InternalEraseQueue|=0x1U<<u16Index; 																	
								bFoundReadyforEraseVirtualSector[u8EEPIndex]=TRUE;
								bFoundReadyForEraseVS[u8EEPIndex] = FALSE;
							}
							else
							{
								if((au32VirtualSectorHeader[2]==0x00000000U) && 
								   (((au32VirtualSectorHeader[4] == 0x0000FFFFU) && (au32VirtualSectorHeader[5] == 0xFFFFFFFFU)) ||
								    ((au32VirtualSectorHeader[4] == 0x00000000U) && (au32VirtualSectorHeader[5] == 0xFFFFFFFFU))
									))
								{
									bFoundActiveVS[u8EEPIndex] = TRUE;
									continue;
								}
								else if((au32VirtualSectorHeader[2]==0x0000FFFFU))
								{
									bFoundCopyVS[u8EEPIndex] = TRUE;
									/* If erase was started/completed, it means there is a VS ready for erase */
									if((au32VirtualSectorHeader[4] == 0x0000FFFFU) && (au32VirtualSectorHeader[5] == 0xFFFFFFFFU) ||
									   (au32VirtualSectorHeader[4] == 0x00000000U) && (au32VirtualSectorHeader[5] == 0xFFFFFFFFU))
									{
										bFoundReadyforEraseVirtualSector[u8EEPIndex]=TRUE;	
									}
									continue;
								}
								else if((au32VirtualSectorHeader[2]==0x00000000U) && 
								        ((au32VirtualSectorHeader[4] == 0x00000000U) && (au32VirtualSectorHeader[5] == 0x0000FFFFU)))
								{
									bFoundReadyForEraseVS[u8EEPIndex] = TRUE;		
									continue;
								}
								else								   
								{	
									/* Report Invalid Virtual Sector State */																
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_au8VirtualSectorState[u16Index]=VsState_ReadyForErase;
									TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16InternalEraseQueue|=0x1U<<u16Index;																
								}
							}
						}						
					}
					else
					{
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
					}
					u16Index++;										
				} /* While for sectors loop ends here */
				
				/* If there is no error in above, continue with setting up FLASH.*/
				if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus==TI_FEE_OK)&& (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error==Error_Nil))
				{	
					/* Set up the State machine.*/ 
					if((Fapi_initializeFlashBanks((uint32)TI_FEE_OPERATING_FREQUENCY))==Fapi_Status_Success)
					{						
					}
					else
					{
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_SetupStateMachine;
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
					}									
					
					if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus!=TI_FEE_ERROR)
					{
						/* Select which Sectors to enable...all the sectors for the particular Bank are enabled*/					
						Fapi_setActiveFlashBank(Device_FlashDevice.Device_BankInfo[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank].Device_Core); 
						Fapi_enableEepromBankSectors(0xFFFFFFFFU, 0xFFFFFFFFU);						
						/* Enable all 1's check. If enabled, reading of all 1's will not generate ECC errors */
						Device_FlashDevice.Device_BankInfo[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank].Device_ControlRegister->FedAcCtrl1.FEDACCTRL1_BITS.EOCV = 0x01U;
						Device_FlashDevice.Device_BankInfo[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank].Device_ControlRegister->EeCtrl1.EE_CTRL1_BITS.EE_ALL1_OK = 0x01U;
						/* Enable SECDED */
						Device_FlashDevice.Device_BankInfo[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank].Device_ControlRegister->FedAcCtrl1.FEDACCTRL1_BITS.EDACEN = 0xAU;
						Device_FlashDevice.Device_BankInfo[TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8Bank].Device_ControlRegister->FedAcCtrl1.FEDACCTRL1_BITS.EDACMODE = 0xAU;
						/* If Ready for Erase and Copy VS found, then mark Copy as Active */
						if((bFoundReadyforEraseVirtualSector[u8EEPIndex]==TRUE) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector == 0U))
						{							
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector=TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector=0U;
							bFoundActiveVirtualSector[u8EEPIndex]=TRUE;
							
							/* Determine the Start & End address of the Active virtual sector */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress=oCopyVirtualSectorStartAddress;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress=oCopyVirtualSectorStartAddress;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorStartAddress = oCopyVirtualSectorStartAddress;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorEndAddress = TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCopyVirtualSectorEndAddress;
							/* Mark the VS Header as Active*/
							TI_FeeInternal_WriteVirtualSectorHeader((uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector,VsState_Active, u8EEPIndex);
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextActiveVSwriteaddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress+TI_FEE_VIRTUAL_SECTOR_OVERHEAD+16U;
						}

						/* If there is one Copy VS and no Active VS and no ready for erase VS, there may be some error */
						if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U)
						{
							if((bFoundReadyforEraseVirtualSector[u8EEPIndex]==FALSE) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector==0U))
							{
								/* Update which of the VS's are Copy */
								TI_Fee_u16ActCpyVS = 0x1U << (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector-1U);
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_CopyButNoActiveVS;								
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
							}
						}						
						
						if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector!=0U) && (TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U))
						{	
							/* One active VS and one copy VS found */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u8WriteCopyVSHeader=TI_FEE_VIRTUAL_SECTOR_OVERHEAD;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oCopyVirtualSectorAddress=oCopyVirtualSectorStartAddress;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress=oCopyVirtualSectorStartAddress;
							/* Update Block offset array. Also update block copy stauus */
							/* Scan Active VS */
							TI_FeeInternal_UpdateBlockOffsetArray(u8EEPIndex, (boolean)TRUE,TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector);
							bActiveVSScanned[u8EEPIndex] = TRUE;
							/* Scan Copy VS */
							TI_FeeInternal_UpdateBlockOffsetArray(u8EEPIndex, (boolean)FALSE,TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector);	
							TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Copy=1U;
							/* Complete copy operation and only then accept new jobs.*/
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_PENDING;
						}
						else if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector==0U) /* No Active Virtual Sector found */
						{
							 /* Create a new Active Virtual Sector */
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector=TI_FeeInternal_FindNextVirtualSector(u8EEPIndex);

							if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector!=0U)&&( TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error==Error_Nil))
							{
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress;
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorStartAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress;
								TI_FeeInternal_WriteVirtualSectorHeader((uint16)TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector,VsState_Active,u8EEPIndex);
								TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextActiveVSwriteaddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress+TI_FEE_VIRTUAL_SECTOR_OVERHEAD+16U;								
								bFoundActiveVirtualSector[u8EEPIndex]=TRUE;
							}							
						}
						else
						{
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress;
							TI_Fee_GlobalVariables[u8EEPIndex].Fee_oActiveVirtualSectorStartAddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_oVirtualSectorStartAddress;
						}
					}
				}				
				/* If Active virtual sector found */
				if(bFoundActiveVirtualSector[u8EEPIndex]==TRUE)				
				{				
					/* Scan Active VS, if it is not already scanned. */
					if(FALSE == bActiveVSScanned[u8EEPIndex])
					{
						TI_FeeInternal_UpdateBlockOffsetArray(u8EEPIndex, (boolean)TRUE,TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector);
					}	
					/* Update the next free address */
					if(TI_Fee_oStatusWord[u8EEPIndex].Fee_StatusWordType_ST.Copy==1U)
					{
						/* Copying is in progress. Next data should be written in to copy VS */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextCopyVSwriteaddress;
					}
					else
					{
						/* Copying is not in progress. Next data should be written in to Active VS */
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress=TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextActiveVSwriteaddress;
					}					
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress=TI_FeeInternal_AlignAddressForECC(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u32nextwriteaddress);										
				}				

				if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16CopyVirtualSector!=0U)
				{
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState = BUSY_INTERNAL;
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult = JOB_PENDING;
				}
				else if(TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16ActiveVirtualSector!=0U)
				{
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState=IDLE;
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult=JOB_OK;
					#if((TI_FEE_FLASH_ERROR_CORRECTION_HANDLING == TI_Fee_Fix))									
					Fee_bSingleBitError = FALSE;
					Fee_bDoubleBitError = FALSE;
					#endif
				}
				else
				{					
					if((TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_TwoActiveVS) &&
					   (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_TwoCopyVS) &&
					   (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_CopyButNoActiveVS) &&
					   (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_SetupStateMachine) &&
					   (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_EraseVS) &&
					   (TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error != Error_NoFreeVS))
					{	
						TI_Fee_GlobalVariables[u8EEPIndex].Fee_Error=Error_NoActiveVS;
					}	
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_oStatus=TI_FEE_ERROR;
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_ModuleState=UNINIT;
					TI_Fee_GlobalVariables[u8EEPIndex].Fee_u16JobResult=JOB_FAILED;
				}				
			}		
		}		
		u8EEPIndex++;
	}
}

#define FEE_STOP_SEC_CODE
#include "MemMap.h"
/**********************************************************************************************************************
 *  END OF FILE: ti_fee_ini.c
 *********************************************************************************************************************/
