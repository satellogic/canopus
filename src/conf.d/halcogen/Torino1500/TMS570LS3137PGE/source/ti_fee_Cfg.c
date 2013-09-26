/** @file TI_Fee_Cfg.c 
*   @brief FEE Driver Implementation File
*   @date 25.July.2013
*   @version 03.06.00
*
*   This is the FEE configuration parameter file.
*
*   (c) Texas Instruments 2009-2013, All rights reserved.
*/



/* Include Files                                                              */

#include "ti_fee.h"



/*----------------------------------------------------------------------------*/
/* Fee Version Check                                                          */

 #if (TI_FEE_MAJOR_VERSION != 3U)
     #error TI_FEE_Cfg.c: TI_FEE_SW_MAJOR_VERSION of TI_FEE.h is incompatible.
 #endif /* FEE_SW_MAJOR_VERSION */
 #if (TI_FEE_MINOR_VERSION != 0U)
     #error TI_FEE_Cfg.c: TI_FEE_SW_MINOR_VERSION of TI_FEE.h is incompatible.
 #endif /* FEE_SW_MINOR_VERSION */
 #if (TI_FEE_PATCH_VERSION != 2U)
     #error TI_FEE_Cfg.c: TI_FEE_SW_PATCH_VERSION of TI_FEE.h is incompatible.
 #endif /* FEE_SW_PATCH_VERSION */

#define FEE_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h" 

/*----------------------------------------------------------------------------*/
/* TI Fee Configurations                                                     */

/** @struct Fee_VirtualSectorConfigType 	*/
/*  @brief Virtual Sector Configuration 	*/
const Fee_VirtualSectorConfigType Fee_VirtualSectorConfiguration[] =
{

    /* Virtual Sector 1 */
    {
       /* Virtual sector number */      1U,
       /* Bank                  */      7U,      
       /* Start Sector          */      (Fapi_FlashSectorType)0U,
       /* End Sector            */      (Fapi_FlashSectorType)0U
    },
    /* Virtual Sector 2 */
    {
        /* Virtual sector number */     2U,
        /* Bank                  */     7U,
        /* Start Sector          */     (Fapi_FlashSectorType)1U,            
		/* End Sector            */     (Fapi_FlashSectorType)1U
    }
  
};


/* Block Configurations         */
const Fee_BlockConfigType Fee_BlockConfiguration[] =
{
        /*      Block 1 */
        {
               /* Block number                          */     1U, 
               /* Block size                            */     512U,
               /* Block immediate data used             */     TRUE,			   
               /* Number of write cycles                */     0x8U,
               /* Device Index                          */     0x00000000U,
               /* Number of DataSets                    */     1U,			   
               /* EEP number                            */     0U			   
        }

 
};

#define FEE_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*----------------------------------------------------------------------------*/
