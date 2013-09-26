/** @file Device_TMS570LS31.c
*   @brief This file defines the FLASH details.
*   @date 25.July.2013
*   @version 03.06.00
*
*/

/* (c) Texas Instruments 2009-2013, All rights reserved. */


/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"
#include "Device_TMS570LS31.h"

/* Start Device Definition */
/*SAFETYMCUSW 79 S MR:19.4 <REVIEWED> "Reason - F021_CPU0_BASE_ADDRESS is a symbolic constant"*/
#define DEVICE_BANKCONTROLREGISTER  F021_CPU0_BASE_ADDRESS  /* Control Register Address */

#define FEE_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

const Device_FlashType Device_FlashDevice =
{
   "TMS570LS31x",                      	/* Device name */
   0x00000000U,                         /* Device Engineering ID */
   Device_ErrorHandlingEcc,             /* Indicates which type of bit Error handling is on the device */   
   Device_CortexR4,					    /* Indicates the Master core type on the device */	   
   TRUE,                              	/* Indicates if the device supports Flash interrupts for processing Flash */   
   31U,                                 /* Nominal time for one write command operation in uS - This value still needs to be characterized */
   300U,                                /* Maximum time for one write command operation in uS - This value still needs to be characterized */       
   {                                    /* Array of Banks on the device */
	   {    /* Start of Bank Definition */
			 /*SAFETYMCUSW 440 S MR:11.3 <REVIEWED> "Reason -  Casting is required here."*/
			 DEVICE_BANKCONTROLREGISTER,	   	/* Pointer to the Flash control register for this bank */
			 Fapi_FlashBank7,               	/* Core number for this bank */                
			{                           /* Array of the Sectors within a bank */
				{   /* Start of Sector Definition */
					Fapi_FlashSector0,   /* Sector number */
					0xF0200000U,         /* Starting address of the sector */
					0x00004000U,         /* Length of the sector */
					100000U,              /* Number of cycles the sector is rated for */
					0xF0100000U,		 /* Defines the address offset to the Error Handling address */
					0x00000800U			 /* Length of the ECC for a sector */
				},  /* End of Sector Definition */
				{   /* Start of Sector Definition */
					Fapi_FlashSector1,   /* Sector number */
					0xF0204000U,         /* Starting address of the sector */
					0x00004000U,         /* Length of the sector */
					100000U,              /* Number of cycles the sector is rated for */
					0xF0100800U,		 /* Defines the address offset to the Error Handling address */
					0x00000800U			 /* Length of the ECC for a sector */
				},  /* End of Sector Definition */
				{   /* Start of Sector Definition */
					Fapi_FlashSector2,   /* Sector number */
					0xF0208000U,         /* Starting address of the sector */
					0x00004000U,         /* Length of the sector */
					100000U,              /* Number of cycles the sector is rated for */
					0xF0101000U,		 /* Defines the address offset to the Error Handling address */
					0x00000800U			 /* Length of the ECC for a sector */
				},  /* End of Sector Definition */
				{   /* Start of Sector Definition */
					Fapi_FlashSector3,   /* Sector number */
					0xF020C000U,         /* Starting address of the sector */
					0x00004000U,         /* Length of the sector */
					100000U,              /* Number of cycles the sector is rated for */
					0xF0101800U,		 /* Defines the address offset to the Error Handling address */
					0x00000800U			 /* Length of the ECC for a sector */
				}  /* End of Sector Definition */
			}
	   }  /* End of Bank Definition */
   }   /* End of Bank Array */
};  /* End of Device Definition */

#define FEE_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* End of File */
