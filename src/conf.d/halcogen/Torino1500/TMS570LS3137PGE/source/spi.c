/** @file spi.c
*   @brief SPI Driver Implementation File
*   @date 29.May.2013
*   @version 03.05.02
*
*   (c) Texas Instruments 2009-2013, All rights reserved.
*/

/* USER CODE BEGIN (0) */
/* USER CODE END */

#include "spi.h"

/* USER CODE BEGIN (1) */
/* USER CODE END */

/** @struct g_spiPacket
*   @brief globals
*
*/
static struct g_spiPacket
{
	spiDAT1_t g_spiDataFormat;
    uint32  tx_length;
    uint32  rx_length;
    uint16  * txdata_ptr;
    uint16  * rxdata_ptr;
    SpiDataStatus_t tx_data_status;
    SpiDataStatus_t rx_data_status;
} g_spiPacket_t[5U];

/** @fn void spiInit(void)
*   @brief Initializes the SPI Driver
*
*   This function initializes the SPI module.
*/
void spiInit(void)
{
/* USER CODE BEGIN (2) */
/* USER CODE END */

    /** @b initialize @b SPI1 */

    /** bring SPI out of reset */
    spiREG1->GCR0 = 1U;

    /** SPI1 master mode and clock configuration */
    spiREG1->GCR1 = (spiREG1->GCR1 & 0xFFFFFFFCU) | ((1U << 1U)  /* CLOKMOD */
                  | 1U);  /* MASTER */

    /** SPI1 enable pin configuration */
    spiREG1->INT0 = (spiREG1->INT0 & 0xFEFFFFFFU)| (0U << 24U);  /* ENABLE HIGHZ */

    /** - Delays */
    spiREG1->DELAY = (0U << 24U)  /* C2TDELAY */
                   | (1U << 16U)  /* T2CDELAY */
                   | (0U << 8U)  /* T2EDELAY */
                   | 0U;  /* C2EDELAY */

    /** - Data Format 0 */
    spiREG1->FMT0 = (3U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (1U << 17U)  /* clock polarity */
                  | (1U << 16U)  /* clock phase */
                  | (62U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 1 */
    spiREG1->FMT1 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (124U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 2 */
    spiREG1->FMT2 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 3 */
    spiREG1->FMT3 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - set interrupt levels */
    spiREG1->LVL = (0U << 9U)  /* TXINT */
                 | (0U << 8U)  /* RXINT */
                 | (0U << 6U)  /* OVRNINT */
                 | (0U << 4U)  /* BITERR */
                 | (0U << 3U)  /* DESYNC */
                 | (0U << 2U)  /* PARERR */
                 | (0U << 1U) /* TIMEOUT */
                 | (0U);  /* DLENERR */

    /** - clear any pending interrupts */
    spiREG1->FLG |= 0xFFFFU;

    /** - enable interrupts */
    spiREG1->INT0 = (spiREG1->INT0 & 0xFFFF0000U)
				  | (0U << 9U)  /* TXINT */
                  | (0U << 8U)  /* RXINT */
                  | (0U << 6U)  /* OVRNINT */
                  | (0U << 4U)  /* BITERR */
                  | (0U << 3U)  /* DESYNC */
                  | (0U << 2U)  /* PARERR */
                  | (0U << 1U) /* TIMEOUT */
                  | (0U);  /* DLENERR */

    /** @b initialize @b SPI1 @b Port */

    /** - SPI1 Port output values */
    spiREG1->PCDOUT =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port direction */
    spiREG1->PCDIR  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port open drain enable */
    spiREG1->PCPDR  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI1 Port pullup / pulldown selection */
    spiREG1->PCPSL  =  1U       /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - SPI1 Port pullup / pulldown enable*/
    spiREG1->PCDIS  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /* SPI1 set all pins to functional */
    spiREG1->PCFUN  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - Initialize TX and RX data buffer Status */					
    g_spiPacket_t[0U].tx_data_status  = SPI_READY;
    g_spiPacket_t[0U].rx_data_status  = SPI_READY;
					
    /** - Finally start SPI1 */
    spiREG1->GCR1 = (spiREG1->GCR1 & 0xFEFFFFFFU) | (1U << 24U);



    /** @b initialize @b SPI3 */

    /** bring SPI out of reset */
    spiREG3->GCR0 = 1U;

    /** SPI3 master mode and clock configuration */
    spiREG3->GCR1 = (spiREG3->GCR1 & 0xFFFFFFFCU) | ((1U << 1U)  /* CLOKMOD */
                  | 1U);  /* MASTER */

    /** SPI3 enable pin configuration */
    spiREG3->INT0 = (spiREG3->INT0 & 0xFEFFFFFFU) | (0U << 24U);  /* ENABLE HIGHZ */

    /** - Delays */
    spiREG3->DELAY = (1U << 24U)  /* C2TDELAY */
                   | (25U << 16U)  /* T2CDELAY */
                   | (0U << 8U)  /* T2EDELAY */
                   | 0U;  /* C2EDELAY */

    /** - Data Format 0 */
    spiREG3->FMT0 = (30U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (1U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 1 */
    spiREG3->FMT1 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 2 */
    spiREG3->FMT2 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - Data Format 3 */
    spiREG3->FMT3 = (0U << 24U)  /* wdelay */
                  | (0U << 23U)  /* parity Polarity */
                  | (0U << 22U)  /* parity enable */
                  | (0U << 21U)  /* wait on enable */
                  | (0U << 20U)  /* shift direction */
                  | (0U << 17U)  /* clock polarity */
                  | (0U << 16U)  /* clock phase */
                  | (12U << 8U) /* baudrate prescale */
                  | 16U;  /* data word length */

    /** - set interrupt levels */
    spiREG3->LVL = (0U << 9U)  /* TXINT */
                 | (0U << 8U)  /* RXINT */
                 | (0U << 6U)  /* OVRNINT */
                 | (0U << 4U)  /* BITERR */
                 | (0U << 3U)  /* DESYNC */
                 | (0U << 2U)  /* PARERR */
                 | (0U << 1U) /* TIMEOUT */
                 | (0U);  /* DLENERR */

    /** - clear any pending interrupts */
    spiREG3->FLG |= 0xFFFFU;

    /** - enable interrupts */
    spiREG3->INT0 = (spiREG3->INT0 & 0xFFFF0000U)
				  | (0U << 9U)  /* TXINT */
                  | (0U << 8U)  /* RXINT */
                  | (0U << 6U)  /* OVRNINT */
                  | (0U << 4U)  /* BITERR */
                  | (0U << 3U)  /* DESYNC */
                  | (0U << 2U)  /* PARERR */
                  | (0U << 1U) /* TIMEOUT */
                  | (0U);  /* DLENERR */

    /** @b initialize @b SPI3 @b Port */

    /** - SPI3 Port output values */
    spiREG3->PCDOUT =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port direction */
    spiREG3->PCDIR  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port open drain enable */
    spiREG3->PCPDR  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /** - SPI3 Port pullup / pulldown selection */
    spiREG3->PCPSL  =  1U       /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - SPI3 Port pullup / pulldown enable*/
    spiREG3->PCDIS  =  0U        /* SCS[0] */
                    | (0U << 1U)  /* SCS[1] */
                    | (0U << 8U)  /* ENA */
                    | (0U << 9U)  /* CLK */
                    | (0U << 10U)  /* SIMO */
                    | (0U << 11U); /* SOMI */

    /* SPI3 set all pins to functional */
    spiREG3->PCFUN  =  1U        /* SCS[0] */
                    | (1U << 1U)  /* SCS[1] */
                    | (1U << 8U)  /* ENA */
                    | (1U << 9U)  /* CLK */
                    | (1U << 10U)  /* SIMO */
                    | (1U << 11U); /* SOMI */

    /** - Initialize TX and RX data buffer Status */					
    g_spiPacket_t[2U].tx_data_status  = SPI_READY;
    g_spiPacket_t[2U].rx_data_status  = SPI_READY;
					
    /** - Finally start SPI3 */
    spiREG3->GCR1 = (spiREG3->GCR1 & 0xFEFFFFFFU) | (1U << 24U);





}


/** @fn void spiSetFunctional(spiBASE_t *spi, uint32 port)
*   @brief Change functional behavior of pins at runtime.
*   @param[in] spi   - Spi module base address
*   @param[in] port  - Value to write to PCFUN register
*
*   Change the value of the PCFUN register at runtime, this allows to
*   dynamically change the functionality of the SPI pins between functional
*   and GIO mode.
*/
void spiSetFunctional(spiBASE_t *spi, uint32 port)
{
/* USER CODE BEGIN (3) */
/* USER CODE END */

    spi->PCFUN = port;

/* USER CODE BEGIN (4) */
/* USER CODE END */
}


/** @fn uint32 spiReceiveData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * destbuff)
*   @brief Receives Data using polling method
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] destbuff		- Pointer to the destination data (16 bit).
*
*   @return flag register value.
*
*   This function transmits blocksize number of data from source buffer using polling method.
*/
uint32 spiReceiveData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * destbuff)
{
/* USER CODE BEGIN (5) */
/* USER CODE END */

    do
    {
        if((spi->FLG & 0x000000FFU) !=0U)
        {
          break;
        }
		/*SAFETYMCUSW 51 S MR:12.3 <REVIEWED> "Needs shifting for 32-bit value" */
		spi->DAT1 = (uint32) (dataconfig_t->DFSEL   << 24U) |
                              (dataconfig_t->CSNR    << 16U) |
                              (dataconfig_t->WDEL    << 26U) |
                              (dataconfig_t->CS_HOLD << 28U) |
                              (0x00000000U);
       while((spi->FLG & 0x00000100U) != 0x00000100U)
	   { 
	   } /* Wait */
       *destbuff++ = spi->BUF;
    }while(blocksize-- > 1U);

/* USER CODE BEGIN (6) */
/* USER CODE END */

    return (spi->FLG & 0xFFU);
}


/** @fn uint32 spiGetData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * destbuff)
*   @brief Receives Data using interrupt method
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] destbuff		- Pointer to the destination data (16 bit).
*
*   @return flag register value.
*
*   This function transmits blocksize number of data from source buffer using interrupt method.
*/
void spiGetData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * destbuff)
{
	 uint32 index = spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));

/* USER CODE BEGIN (7) */
/* USER CODE END */

     g_spiPacket_t[index].rx_length = blocksize;
     g_spiPacket_t[index].rxdata_ptr   = destbuff;
     g_spiPacket_t[index].g_spiDataFormat = *dataconfig_t;
     g_spiPacket_t[index].rx_data_status = SPI_PENDING;

     spi->INT0 |= 0x0100U;

/* USER CODE BEGIN (8) */
/* USER CODE END */
}


/** @fn uint32 spiTransmitData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff)
*   @brief Transmits Data using polling method
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] srcbuff		- Pointer to the source data ( 16 bit).
*
*   @return flag register value.
*
*   This function transmits blocksize number of data from source buffer using polling method.
*/
uint32 spiTransmitData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff)
{

    volatile uint32 SpiBuf;
	uint32 Chip_Select_Hold = 0U;

/* USER CODE BEGIN (9) */
/* USER CODE END */

    do
    {
        if((spi->FLG & 0x000000FFU) !=0U)
        {
           break;
        }
   	   
		if(blocksize <= 2)
		{
		   Chip_Select_Hold = 0U;
		}
        else
		{
		   Chip_Select_Hold = dataconfig_t->CS_HOLD << 28U;
		}

        /*SAFETYMCUSW 51 S MR:12.3 <REVIEWED> "Needs shifting for 32-bit value" */
 		spi->DAT1 = (uint32) (dataconfig_t->DFSEL   << 24U)  |
                              (dataconfig_t->CSNR    << 16U) |
                              (dataconfig_t->WDEL    << 26U) |
                              (Chip_Select_Hold) 			 |
                              (*srcbuff++);
       while((spi->FLG & 0x00000100U) != 0x00000100U)
	   { 
	   } /* Wait */
       SpiBuf = spi->BUF;
    
    }while(blocksize-- > 1U);

/* USER CODE BEGIN (10) */
/* USER CODE END */

    return (spi->FLG & 0xFFU);
}


/** @fn void spiSendData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff)
*   @brief Transmits Data using interrupt method
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] srcbuff		- Pointer to the source data ( 16 bit).
*
*   @return flag register value.
*
*   This function transmits blocksize number of data from source buffer using interrupt method.
*/
void spiSendData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff)
{
   	 uint32 index = spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));

/* USER CODE BEGIN (11) */
/* USER CODE END */

     g_spiPacket_t[index].tx_length = blocksize;
     g_spiPacket_t[index].txdata_ptr   = srcbuff;
     g_spiPacket_t[index].g_spiDataFormat = *dataconfig_t;
     g_spiPacket_t[index].tx_data_status = SPI_PENDING;

     spi->INT0 |= 0x0200U;

/* USER CODE BEGIN (12) */
/* USER CODE END */
}


/** @fn uint32 spiTransmitAndReceiveData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff, uint16 * destbuff)
*   @brief Transmits and Receive Data using polling method
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] srcbuff		- Pointer to the source data ( 16 bit).
*   @param[in] destbuff		- Pointer to the destination data ( 16 bit).
*
*   @return flag register value.
*
*   This function transmits and receives blocksize number of data from source buffer using polling method.
*/
uint32 spiTransmitAndReceiveData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff, uint16 * destbuff)
{

	uint32 Chip_Select_Hold = 0U;

/* USER CODE BEGIN (13) */
/* USER CODE END */

    do
    {
        if((spi->FLG & 0x000000FFU) !=0U)
        {
           break;
        }
   	   
		if(blocksize <= 2)
		{
		   Chip_Select_Hold = 0U;
		}
        else
		{
		   Chip_Select_Hold = dataconfig_t->CS_HOLD << 28U;
		}	   

		/*SAFETYMCUSW 51 S MR:12.3 <REVIEWED> "Needs shifting for 32-bit value" */
       spi->DAT1 = (uint32) (dataconfig_t->DFSEL   << 24U)   |
                              (dataconfig_t->CSNR    << 16U) |
                              (dataconfig_t->WDEL    << 26U) |
                              (Chip_Select_Hold)             |
                              (*srcbuff++);
       while((spi->FLG & 0x00000100U) != 0x00000100U)
	   { 
	   } /* Wait */
       *destbuff++ = spi->BUF;
    
    }while(blocksize-- > 1U);

/* USER CODE BEGIN (14) */
/* USER CODE END */

    return (spi->FLG & 0xFFU);
}

/* USER CODE BEGIN (15) */
/* USER CODE END */

/** @fn void spiSendAndGetData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff, uint16 * destbuff)
*   @brief Initiate SPI Transmits and receive Data using Interrupt mode.
*   @param[in] spi   		- Spi module base address
*   @param[in] dataconfig_t	- Spi DAT1 register configuration
*   @param[in] blocksize	- number of data
*   @param[in] srcbuff		- Pointer to the source data ( 16 bit).
*   @param[in] destbuff		- Pointer to the destination data ( 16 bit).
*
*   Initiate SPI Transmits and receive Data using Interrupt mode..
*/
void spiSendAndGetData(spiBASE_t *spi, spiDAT1_t *dataconfig_t, uint32 blocksize, uint16 * srcbuff, uint16 * destbuff)
{

/* USER CODE BEGIN (16) */
/* USER CODE END */

	uint32 index = spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));

    g_spiPacket_t[index].tx_length       = blocksize;
    g_spiPacket_t[index].rx_length       = blocksize;
    g_spiPacket_t[index].txdata_ptr      = srcbuff;
    g_spiPacket_t[index].rxdata_ptr      = destbuff;
    g_spiPacket_t[index].g_spiDataFormat = *dataconfig_t;
    g_spiPacket_t[index].tx_data_status  = SPI_PENDING;
    g_spiPacket_t[index].rx_data_status  = SPI_PENDING;

    spi->INT0 |= 0x0300U;

/* USER CODE BEGIN (17) */
/* USER CODE END */
}

/** @fn SpiDataStatus_t SpiTxStatus(spiBASE_t *spi)
*   @brief Get the status of the SPI Transmit data block.
*   @param[in] spi   		- Spi module base address
*
*   @return Spi Transmit block data status.
*
*   Get the status of the SPI Transmit data block.
*/
SpiDataStatus_t SpiTxStatus(spiBASE_t *spi)
{

/* USER CODE BEGIN (18) */
/* USER CODE END */

    uint32 index = spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));
	return(g_spiPacket_t[index].tx_data_status);
}

/* USER CODE BEGIN (19) */
/* USER CODE END */

/** @fn SpiDataStatus_t SpiRxStatus(spiBASE_t *spi)
*   @brief Get the status of the SPI Receive data block.
*   @param[in] spi   		- Spi module base address
*
*   @return Spi Receive block data status.
*
*   Get the status of the SPI Receive data block.
*/
SpiDataStatus_t SpiRxStatus(spiBASE_t *spi)
{

/* USER CODE BEGIN (20) */
/* USER CODE END */

    uint32 index = spi == spiREG1 ? 0U :(spi==spiREG2 ? 1U : (spi==spiREG3 ? 2U:(spi==spiREG4 ? 3U:4U)));
	return(g_spiPacket_t[index].rx_data_status);
}

/* USER CODE BEGIN (21) */
/* USER CODE END */

/** @fn void spiEnableLoopback(spiBASE_t *spi, loopBackType_t Loopbacktype)
*   @brief Enable Loopback mode for self test
*   @param[in] spi        - spi module base address
*   @param[in] Loopbacktype  - Digital or Analog
*
*   This function enables the Loopback mode for self test.
*/
void spiEnableLoopback(spiBASE_t *spi, loopBackType_t Loopbacktype)
{
/* USER CODE BEGIN (22) */
/* USER CODE END */
    
	/* Clear Loopback incase enabled already */
	spi->IOLPKTSTCR = 0U;
	
	/* Enable Loopback either in Analog or Digital Mode */
    spi->IOLPKTSTCR = 0x00000A00U
                       | (Loopbacktype << 1U);
	
/* USER CODE BEGIN (23) */
/* USER CODE END */
}

/* USER CODE BEGIN (24) */
/* USER CODE END */

/** @fn void spiDisableLoopback(spiBASE_t *spi)
*   @brief Enable Loopback mode for self test
*   @param[in] spi        - spi module base address
*
*   This function disable the Loopback mode.
*/
void spiDisableLoopback(spiBASE_t *spi)
{
/* USER CODE BEGIN (25) */
/* USER CODE END */
    
	/* Disable Loopback Mode */
    spi->IOLPKTSTCR = 0x00000500U;
	
/* USER CODE BEGIN (26) */
/* USER CODE END */
}

/* USER CODE BEGIN (27) */
/* USER CODE END */

/** @fn spiEnableNotification(spiBASE_t *spi, uint32 flags)
*   @brief Enable interrupts
*   @param[in] spi   - spi module base address
*   @param[in] flags - Interrupts to be enabled, can be ored value of:
*/
void spiEnableNotification(spiBASE_t *spi, uint32 flags)
{
/* USER CODE BEGIN (28) */
/* USER CODE END */

    spi->INT0 = (spi->INT0 & 0xFFFF0000U) | flags;

/* USER CODE BEGIN (29) */
/* USER CODE END */
}

/* USER CODE BEGIN (30) */
/* USER CODE END */

/** @fn spiDisableNotification(spiBASE_t *spi, uint32 flags)
*   @brief Enable interrupts
*   @param[in] spi   - spi module base address
*   @param[in] flags - Interrupts to be enabled, can be ored value of:
*/
void spiDisableNotification(spiBASE_t *spi, uint32 flags)
{
/* USER CODE BEGIN (31) */
/* USER CODE END */

    spi->INT0 = ((spi->INT0 & 0x0000FFFFU) & (~(flags)));

/* USER CODE BEGIN (32) */
/* USER CODE END */
}


/* USER CODE BEGIN (37) */
/* USER CODE END */

/** @fn void mibspi1HighLevelInterrupt(void)
*   @brief Level 0 Interrupt for SPI1
*/
#pragma CODE_STATE(mibspi1HighLevelInterrupt, 32)
#pragma INTERRUPT(mibspi1HighLevelInterrupt, IRQ)
void mibspi1HighLevelInterrupt(void)
{

/* USER CODE BEGIN (38) */
/* USER CODE END */

    uint32 vec = spiREG1->INTVECT0;
    uint32 flags = (spiREG1->FLG & 0x0000FFFFU) & (~spiREG1->LVL & 0x035FU);

/* USER CODE BEGIN (39) */
/* USER CODE END */

    switch(vec)
    {

    case 0x24U: /* Receive Buffer Full Interrupt */
             {
                *g_spiPacket_t[0U].rxdata_ptr++ = spiREG1->BUF;
                g_spiPacket_t[0U].rx_length--;

                if(g_spiPacket_t[0U].rx_length == 0U)
                {
                    spiREG1->INT0 = (spiREG1->INT0 & 0x0000FFFFU) & ~(0x0100U);
                    g_spiPacket_t[0U].rx_data_status = SPI_COMPLETED;
                    spiEndNotification(spiREG1);
                }
             }
    break;

    case 0x28U: /* Transmit Buffer Empty Interrupt */
             {
            	 volatile uint32 SpiBuf;
				 uint32 Chip_Select_Hold = 0U;

            	 g_spiPacket_t[0U].tx_length--;

				 if(g_spiPacket_t[0U].tx_length == 1U)
				 {
				    Chip_Select_Hold = 0U;
				 }
                 else
				 {
				    Chip_Select_Hold = g_spiPacket_t[0U].g_spiDataFormat.CS_HOLD  << 28U;
				 }
				 
            	 spiREG1->DAT1 = (uint32) ((g_spiPacket_t[0U].g_spiDataFormat.DFSEL   << 24U)  |
                		                   (g_spiPacket_t[0U].g_spiDataFormat.CSNR     << 16U) |
                		                   (g_spiPacket_t[0U].g_spiDataFormat.WDEL     << 26U) |
                		                   (Chip_Select_Hold)                                  |
                                           (*g_spiPacket_t[0U].txdata_ptr++));

            	 /* Dummy Receive read if no RX Interrupt enabled */
            	 if(((spiREG1->INT0 & 0x0000FFFFU)& 0x0100U) == 0U)
                 {
                     if((spiREG1->FLG & 0x00000100U) != 0x00000100U)
                     SpiBuf = spiREG1->BUF;
                 }

            	 if(g_spiPacket_t[0U].tx_length == 0U)
                 {
                    spiREG1->INT0 = (spiREG1->INT0 & 0x0000FFFFU) & ~(0x0200U); /* Disable Interrupt */
                    g_spiPacket_t[0U].tx_data_status = SPI_COMPLETED;
                    spiEndNotification(spiREG1);
                }
             }
    break;

    default: /* Clear Flags and return  */
             spiREG1->FLG = flags;
             spiNotification(spiREG1, flags);
    break;
    }

/* USER CODE BEGIN (40) */
/* USER CODE END */
}



/* USER CODE BEGIN (53) */
/* USER CODE END */

/** @fn void mibspi3HighInterruptLevel(void)
*   @brief Level 0 Interrupt for SPI3
*/
#pragma CODE_STATE(mibspi3HighInterruptLevel, 32)
#pragma INTERRUPT(mibspi3HighInterruptLevel, IRQ)
void mibspi3HighInterruptLevel(void)
{

/* USER CODE BEGIN (54) */
/* USER CODE END */

    uint32 vec = spiREG3->INTVECT0;
    uint32 flags = (spiREG3->FLG & 0x0000FFFFU) & (~spiREG3->LVL & 0x035FU);

/* USER CODE BEGIN (55) */
/* USER CODE END */

    switch(vec)
    {

    case 0x24U: /* Receive Buffer Full Interrupt */
             {
                *g_spiPacket_t[2U].rxdata_ptr++ = spiREG3->BUF;
                g_spiPacket_t[2U].rx_length--;

                if(g_spiPacket_t[2U].rx_length == 0U)
                {
                    spiREG3->INT0 = (spiREG3->INT0 & 0x0000FFFFU) & ~(0x0100U);
                    g_spiPacket_t[2U].rx_data_status = SPI_COMPLETED;
                    spiEndNotification(spiREG3);
                }
             }
    break;

    case 0x28U: /* Transmit Buffer Empty Interrupt */
             {
            	 volatile uint32 SpiBuf;
				 uint32 Chip_Select_Hold = 0U;

            	 g_spiPacket_t[2U].tx_length--;

				 if(g_spiPacket_t[2U].tx_length == 1U)
				 {
				    Chip_Select_Hold = 0U;
				 }
                 else
				 {
				    Chip_Select_Hold = g_spiPacket_t[2U].g_spiDataFormat.CS_HOLD  << 28U;
				 }
				 
            	 spiREG3->DAT1 = (uint32) ((g_spiPacket_t[2U].g_spiDataFormat.DFSEL   << 24U)  |
                		                   (g_spiPacket_t[2U].g_spiDataFormat.CSNR     << 16U) |
                		                   (g_spiPacket_t[2U].g_spiDataFormat.WDEL     << 26U) |
                		                   (Chip_Select_Hold)                                  |
                                           (*g_spiPacket_t[2U].txdata_ptr++));

            	 /* Dummy Receive read if no RX Interrupt enabled */
            	 if(((spiREG3->INT0 & 0x0000FFFFU)& 0x0100U) == 0U)
                 {
                     if((spiREG3->FLG & 0x00000100U) != 0x00000100U)
                     SpiBuf = spiREG3->BUF;
                 }

            	 if(g_spiPacket_t[2U].tx_length == 0U)
                 {
                    spiREG3->INT0 = (spiREG3->INT0 & 0x0000FFFFU) & ~(0x0200U); /* Disable Interrupt */
                    g_spiPacket_t[2U].tx_data_status = SPI_COMPLETED;
                    spiEndNotification(spiREG3);
                }
             }
    break;

    default: /* Clear Flags and return  */
             spiREG3->FLG = flags;
             spiNotification(spiREG3, flags);
    break;
    }

/* USER CODE BEGIN (56) */
/* USER CODE END */
}




