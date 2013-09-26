/** @file Device_types.h
*   @brief This file defines the structures for FLASH details
*   @date 29.May.2013
*   @version 03.05.02
*   
*/

/* (c) Texas Instruments 2009-2013, All rights reserved. */


#ifndef DEVICE_TYPES_H
#define DEVICE_TYPES_H

#include "Std_Types.h"

/* Enum to describe the type of error handling on the device */
typedef enum
{
   Device_ErrorHandlingNone,                /* Device has no error handling */
   Device_ErrorHandlingParity,              /* Device has parity error handling */
   Device_ErrorHandlingEcc                  /* Device has ECC error handling */
} Device_FlashErrorCorrectionProcessType;

/* Enum to describe the ARM core on the device*/
typedef enum
{
	Device_CoreNone,      /* To indicate that the device has a single core */
	Device_Arm7,		  /* To indicate that the device has a ARM7 core	*/
	Device_CortexR4,	  /* To indicate that the device has a CortexR4 core */
	Device_CortexM3	      /* To indicate that the device has a CortexM3 core */
}Device_ArmCoreType;

/* Structure defines an individual sector within a bank */
typedef struct
{   
   Fapi_FlashSectorType Device_Sector;										   /* Sector number */ 
   uint32 Device_SectorStartAddress;                                           /* Starting address of the sector */
   uint32 Device_SectorLength;                                                 /* Length of the sector */
   uint32 Device_MaxWriteCycles;                                               /* Number of cycles the sector is rated for */
   uint32 Device_EccAddress;
   uint32 Device_EccLength;
} Device_SectorType;

/* Structure defines an individual bank */
typedef struct
{  
   Fapi_FmcRegistersType * Device_ControlRegister;	
   Fapi_FlashBankType Device_Core;                                                     /* Core number for this bank */       
   Device_SectorType Device_SectorInfo[DEVICE_BANK_MAX_NUMBER_OF_SECTORS];             /* Array of the Sectors within a bank */
} Device_BankType;

/* Structure defines the Flash structure of the device */
typedef struct
{
   uint8 Device_DeviceName[12];                                                  /* Device name */
   uint32 Device_EngineeringId;                                                  /* Device Engineering ID */
   Device_FlashErrorCorrectionProcessType Device_FlashErrorHandlingProcessInfo;  /* Indicates which type of bit Error handling is on the device */   
   Device_ArmCoreType	Device_MasterCore;									     /* Indicates the Master core type on the device */   
   boolean Device_SupportsInterrupts;                                            /* Indicates if the device supports Flash interrupts for processing Flash */   
   uint32 Device_NominalWriteTime;                                               /* Nominal time for one write command operation in uS */
   uint32 Device_MaximumWriteTime;                                               /* Maximum time for one write command operation in uS */ 
   Device_BankType Device_BankInfo[DEVICE_NUMBER_OF_FLASH_BANKS];                /* Array of Banks on the device */
} Device_FlashType;

#endif /* DEVICE_TYPES_H */

/* End of File */
