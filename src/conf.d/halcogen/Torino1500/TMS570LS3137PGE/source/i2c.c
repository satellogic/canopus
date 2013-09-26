/** @file i2c.c 
*   @brief I2C Driver Implementation File
*   @date 29.May.2013
*   @version 03.05.02
*
*/

/* (c) Texas Instruments 2009-2013, All rights reserved. */

/* USER CODE BEGIN (0) */
/* USER CODE END */

#include "i2c.h"

/* USER CODE BEGIN (1) */
/* USER CODE END */

/** @struct g_i2CTransfer
*   @brief Interrupt mode globals
*
*/
static struct g_i2cTransfer
{
    uint32  mode;
    uint32  length;
    uint8   * data;
} g_i2cTransfer_t[2U];

/* USER CODE BEGIN (2) */
/* USER CODE END */

/** @fn void i2cInit(void)
*   @brief Initializes the i2c Driver
*
*   This function initializes the i2c module.
*/
void i2cInit(void)
{
/* USER CODE BEGIN (3) */
/* USER CODE END */

    /** @b initialize @b I2C */

    /** - i2c out of reset */
    i2cREG1->MDR = (1U << 5U);

    /** - set i2c mode */
    i2cREG1->MDR =   (0U << 15U)     /* nack mode                        */           
                   | (0U << 14U)     /* free running                      */           
                   |  0U     /* start condition - master mode only */    
                   | (1U <<11U)     /* stop condition                     */ 
                   | (1U <<10U)     /* Master/Slave mode                 */ 
                   | (I2C_TRANSMITTER)     /* Transmitter/receiver              */ 
                   | (I2C_7BIT_AMODE)     /* xpanded address                   */ 
                   | (0 << 7U)     /* repeat mode                       */ 
                   | (0U << 6U)     /* digital loopback                  */          
                   | (0U << 4U)     /* start byte - master only          */ 
                   | (0U)     /* free data format                  */ 
                   | (I2C_8_BIT);     /* bit count                         */ 


    /** - set i2c extended mode */
    i2cREG1->EMDR = (0U << 25U);

    /** - set i2c data count */
    i2cREG1->CNT = 1U;

    /** - disable all interrupts */
    i2cREG1->IMR = 0x00U;    

    /** - set prescale */
    i2cREG1->PSC = 1U;

    /** - set clock rate */
    i2cREG1->CLKH = 307U;
    i2cREG1->CLKL = 307U;

    /** - set i2c pins functional mode */
    i2cREG1->FUN = (0U);

    /** - set i2c pins default output value */
    i2cREG1->DOUT = (0U << 1U)     /* sda pin */
                  | (0U);     /* scl pin */

    /** - set i2c pins output direction */
    i2cREG1->DIR = (0U << 1U)     /* sda pin */
                 | (0U);     /* scl pin */

    /** - set i2c pins open drain enable */
    i2cREG1->ODR = (0U << 1U)     /* sda pin */
                 | (0U);     /* scl pin */

    /** - set i2c pins pullup/pulldown enable */
    i2cREG1->PD = (1U << 1U)     /* sda pin */
                | (1U);     /* scl pin */

    /** - set i2c pins pullup/pulldown select */
    i2cREG1->PSL = (1U << 1U)     /* sda pin */
                 | (1U);     /* scl pin */

    i2cREG1->MDR |= I2C_RESET_OUT; /* i2c out of reset */

	/** - initialize global transfer variables */
    g_i2cTransfer_t[0U].mode   = 1U << 8U;
    g_i2cTransfer_t[0U].length = 0U;
    g_i2cTransfer_t[1U].mode   = 1U << 8U;
    g_i2cTransfer_t[1U].length = 0U;

    /** - set interrupt enable */
    i2cREG1->IMR    = (0U << 6U)     /* Address as slave interrupt      */
                    | (0U << 5U)     /* Stop Condition detect interrupt */
                    | (1U << 4U)     /* Transmit data ready interrupt   */
                    | (1U << 3U)     /* Receive data ready interrupt    */
                    | (0U << 2U)     /* Register Access ready interrupt */
                    | (1U << 1U)     /* No Acknowledgment interrupt    */
                    | (1U);     /* Arbitration Lost interrupt      */
    
/* USER CODE BEGIN (4) */
/* USER CODE END */
}

/** @fn void i2cSetOwnAdd(i2cBASE_t *i2c, uint32 oadd)
*   @brief Set I2C Own Address
*   @param[in] oadd - I2C Own address (7-bit or 10 -bit address)
*   @param[in] i2c  - i2c module base address
*   Set the Own address of the I2C module.
*/
void i2cSetOwnAdd(i2cBASE_t *i2c, uint32 oadd)
{
    i2cREG1->OAR = oadd;  /* set own address */
}

/** @fn void i2cSetSlaveAdd(i2cBASE_t *i2c, uint32 sadd)
*   @brief Set Port Direction
*   @param[in] sadd - I2C Slave address
*   @param[in] i2c  - i2c module base address
*   Set the Slave address to communicate which is must in Master mode.
*/
void i2cSetSlaveAdd(i2cBASE_t *i2c, uint32 sadd)
{
    i2cREG1->SAR = sadd;  /* set slave address */
}

/** @fn void i2cSetBaudrate(i2cBASE_t *i2c, uint32 baud)
*   @brief Change baudrate at runtime.
*   @param[in] i2c  - i2c module base address
*   @param[in] baud - baudrate in KHz
*
*   Change the i2c baudrate at runtime.
*/
void i2cSetBaudrate(i2cBASE_t *i2c, uint32 baud)
{
    uint32 prescale;
    uint32 d;    
    uint32 ck;    
    float64   vclk = 12.500 * 1000000.0;

/* USER CODE BEGIN (5) */
/* USER CODE END */
    prescale = (uint32) ((vclk /8000000U) - 1U);

    if(prescale>=2U)
    {
	d = 5U;
    }
    else
    {
	d = (prescale != 0U) ? 6U : 7U;
    }

	/*SAFETYMCUSW 96 S MR:6.1 <REVIEWED> "Calculations including int and float cannot be avoided" */
    ck = ((vclk)/(2U*baud*1000U*(prescale+1U)))-d;

    i2cREG1->PSC  = prescale;
    i2cREG1->CLKH = ck;
    i2cREG1->CLKL = ck;    

/* USER CODE BEGIN (6) */
/* USER CODE END */

}

/** @fn void i2cSetStart(i2cBASE_t *i2c)
*   @brief Set i2c start condition
*   @param[in] i2c  - i2c module base address
*   Set i2c to generate a start bit (Only in Master mode)
*/
void i2cSetStart(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (7) */
/* USER CODE END */

	i2cREG1->MDR |= I2C_START_COND;  /* set start condition */

/* USER CODE BEGIN (8) */
/* USER CODE END */
}

/** @fn void i2cSetStop(i2cBASE_t *i2c)
*   @brief Set i2c stop condition
*   @param[in] i2c  - i2c module base address
*   Set i2c to generate a stop bit (Only in Master mode)
*/
void i2cSetStop(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (9) */
/* USER CODE END */

	i2cREG1->MDR |= I2C_STOP_COND;  /* generate stop condition */

/* USER CODE BEGIN (10) */
/* USER CODE END */
}

/** @fn void i2cSetCount(i2cBASE_t *i2c,uint32 cnt)
*   @brief Set i2c data count
*   @param[in] i2c  - i2c module base address
*   @param[in] cnt  - data count
*   Set i2c count to a transfer value after which the stop condition needs to be generated.
*   (Only in Master Mode)
*/
void i2cSetCount(i2cBASE_t *i2c ,uint32 cnt)
{
/* USER CODE BEGIN (11) */
/* USER CODE END */

	i2cREG1->CNT = cnt;  /* set i2c count  */

/* USER CODE BEGIN (12) */
/* USER CODE END */
}

/** @fn uint32 i2cIsTxReady(i2cBASE_t *i2c)
*   @brief Check if Tx buffer empty
*   @param[in] i2c - i2c module base address
*
*   @return The TX ready flag
*
*   Checks to see if the Tx buffer ready flag is set, returns
*   0 is flags not set otherwise will return the Tx flag itself.
*/
uint32 i2cIsTxReady(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (13) */
/* USER CODE END */

    return i2cREG1->STR & I2C_TX_INT;

/* USER CODE BEGIN (14) */
/* USER CODE END */
}

/** @fn void i2cSendByte(i2cBASE_t *i2c, uint8 byte)
*   @brief Send Byte
*   @param[in] i2c  - i2c module base address
*   @param[in] byte - byte to transfer
*
*   Sends a single byte in polling mode, will wait in the
*   routine until the transmit buffer is empty before sending
*   the byte.  Use i2cIsTxReady to check for Tx buffer empty
*   before calling i2cSendByte to avoid waiting.
*/
void i2cSendByte(i2cBASE_t *i2c, uint8 byte)
{
/* USER CODE BEGIN (15) */
/* USER CODE END */

    while ((i2cREG1->STR & I2C_TX_INT) == 0U) 
    { 
	} /* Wait */
    i2cREG1->DXR = byte;

/* USER CODE BEGIN (16) */
/* USER CODE END */
}

/** @fn void i2cSend(i2cBASE_t *i2c, uint32 length, uint8 * data)
*   @brief Send Data
*   @param[in] i2c    - i2c module base address
*   @param[in] length - number of data words to transfer
*   @param[in] data   - pointer to data to send
*
*   Send a block of data pointed to by 'data' and 'length' bytes
*   long.  If interrupts have been enabled the data is sent using
*   interrupt mode, otherwise polling mode is used.  In interrupt
*   mode transmission of the first byte is started and the routine
*   returns immediately, i2cSend must not be called again until the
*   transfer is complete, when the i2cNotification callback will
*   be called.  In polling mode, i2cSend will not return  until 
*   the transfer is complete.
*
*   @note if data word is less than 8 bits, then the data must be left
*         aligned in the data byte.
*/
void i2cSend(i2cBASE_t *i2c, uint32 length, uint8 * data)
{
    uint32 index = i2c == i2cREG1 ? 0U : 1U;

/* USER CODE BEGIN (17) */
/* USER CODE END */

    if ((g_i2cTransfer_t[index].mode & I2C_TX_INT) != 0U)
    {
        /* we are in interrupt mode */
        
        g_i2cTransfer_t[index].length = length;
        g_i2cTransfer_t[index].data   = data;

        /* start transmit by sending first byte */        
        i2cREG1->DXR = *(g_i2cTransfer_t[index].data + 1U);
        i2cREG1->IMR = I2C_TX_INT;
    }
    else
    {
        /* send the data */
        while (length-- > 0U)
        {
            while ((i2cREG1->STR & I2C_TX_INT) == 0U) 
		    { 
	        } /* Wait */
            i2cREG1->DXR = *data++;
        }
    }
/* USER CODE BEGIN (18) */
/* USER CODE END */
}

/** @fn uint32 i2cIsRxReady(i2cBASE_t *i2c)
*   @brief Check if Rx buffer full
*   @param[in] i2c - i2c module base address
*
*   @return The Rx ready flag
*
*   Checks to see if the Rx buffer full flag is set, returns
*   0 is flags not set otherwise will return the Rx flag itself.
*/
uint32 i2cIsRxReady(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (19) */
/* USER CODE END */

    return i2cREG1->STR & I2C_RX_INT;

/* USER CODE BEGIN (20) */
/* USER CODE END */
}


/** @fn uint32 i2cRxError(i2cBASE_t *i2c)
*   @brief Return Rx Error flags
*   @param[in] i2c - i2c module base address
*
*   @return The Rx error flags
*
*   Returns the Rx framing, overrun and parity errors flags,
*   also clears the error flags before returning.
*/
uint32 i2cRxError(i2cBASE_t *i2c)
{
    uint32 status = i2cREG1->STR & (I2C_AL_INT | I2C_NACK_INT);

/* USER CODE BEGIN (21) */
/* USER CODE END */

    i2cREG1->STR = I2C_AL_INT | I2C_NACK_INT;
	
/* USER CODE BEGIN (22) */
/* USER CODE END */
    
	return status;

}

/** @fn void i2cClearSCD(i2cBASE_t *i2c)
*   @brief Clears the Stop condition detect flags.
*   @param[in] i2c - i2c module base address
*
*   This function is called to clear the Stop condition detect(SCD) flag
*/
void i2cClearSCD(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (23) */
/* USER CODE END */

    i2cREG1->STR = I2C_SCD_INT;
	
/* USER CODE BEGIN (24) */
/* USER CODE END */
}

/** @fn uint32 i2cReceiveByte(i2cBASE_t *i2c)
*   @brief Receive Byte
*   @param[in] i2c - i2c module base address
*
*   @return Received byte
*
*    Receives a single byte in polling mode.  If there is
*    not a byte in the receive buffer the routine will wait
*    until one is received.   Use i2cIsRxReady to check to
*    see if the buffer is full to avoid waiting.
*/
uint32 i2cReceiveByte(i2cBASE_t *i2c)
{
    while ((i2cREG1->STR & I2C_RX_INT) == 0U) 
    { 
	} /* Wait */
/* USER CODE BEGIN (25) */
/* USER CODE END */

    return i2cREG1->DRR;
}

/** @fn void i2cReceive(i2cBASE_t *i2c, uint32 length, uint8 * data)
*   @brief Receive Data
*   @param[in] i2c    - i2c module base address
*   @param[in] length - number of data words to transfer
*   @param[in] data   - pointer to data buffer
*
*   Receive a block of 'length' bytes long and place it into the 
*   data buffer pointed to by 'data'.  If interrupts have been 
*   enabled the data is received using interrupt mode, otherwise
*   polling mode is used.  In interrupt mode receive is setup and
*   the routine returns immediately, i2cReceive must not be called 
*   again until the transfer is complete, when the i2cNotification 
*   callback will be called.  In polling mode, i2cReceive will not
*   return  until the transfer is complete.
*/
void i2cReceive(i2cBASE_t *i2c, uint32 length, uint8 * data)
{

/* USER CODE BEGIN (26) */
/* USER CODE END */
    if ((i2cREG1->IMR & I2C_RX_INT) != 0U)
    {
        /* we are in interrupt mode */
        uint32 index = i2c == i2cREG1 ? 0U : 1U;
        
        /* clear error flags */
        i2cREG1->STR = I2C_AL_INT | I2C_NACK_INT;

        g_i2cTransfer_t[index].length = length;
        g_i2cTransfer_t[index].data   = data;
    }
    else
    {   
        while (length-- > 0U)
        {
            while ((i2cREG1->STR & I2C_RX_INT) == 0U) 
		    { 
	        } /* Wait */
            *data++ = i2cREG1->DRR;
        }
    }

/* USER CODE BEGIN (27) */
/* USER CODE END */
}

/** @fn void i2cEnableLoopback(i2cBASE_t *i2c)
*   @brief Enable Loopback mode for self test
*   @param[in] i2c        - i2c module base address
*
*   This function enables the Loopback mode for self test.
*/
void i2cEnableLoopback(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (28) */
/* USER CODE END */

    /* enable digital loopback    */
    i2cREG1->MDR |= (1U << 6U); 

/* USER CODE BEGIN (29) */
/* USER CODE END */
}

/** @fn void i2cDisableLoopback(i2cBASE_t *i2c)
*   @brief Enable Loopback mode for self test
*   @param[in] i2c        - i2c module base address
*
*   This function disable the Loopback mode.
*/
void i2cDisableLoopback(i2cBASE_t *i2c)
{
/* USER CODE BEGIN (30) */
/* USER CODE END */
    
	/* Disable Loopback Mode */
    i2cREG1->MDR &= 0xFFFFFFBFU; 

/* USER CODE BEGIN (31) */
/* USER CODE END */
}

/** @fn i2cEnableNotification(i2cBASE_t *i2c, uint32 flags)
*   @brief Enable interrupts
*   @param[in] i2c   - i2c module base address
*   @param[in] flags - Interrupts to be enabled, can be ored value of:
*                      i2c_FE_INT    - framing error,
*                      i2c_OE_INT    - overrun error,
*                      i2c_PE_INT    - parity error,
*                      i2c_RX_INT    - receive buffer ready,
*                      i2c_TX_INT    - transmit buffer ready,
*                      i2c_WAKE_INT  - wakeup,
*                      i2c_BREAK_INT - break detect
*/
void i2cEnableNotification(i2cBASE_t *i2c, uint32 flags)
{
    uint32 index = i2c == i2cREG1 ? 0U : 1U;

/* USER CODE BEGIN (32) */
/* USER CODE END */

    g_i2cTransfer_t[index].mode |= (flags & I2C_TX_INT);
    i2cREG1->IMR               = (flags & (~I2C_TX_INT));
}

/** @fn i2cDisableNotification(i2cBASE_t *i2c, uint32 flags)
*   @brief Disable interrupts
*   @param[in] i2c   - i2c module base address
*   @param[in] flags - Interrupts to be disabled, can be ored value of:
*                      i2c_FE_INT    - framing error,
*                      i2c_OE_INT    - overrun error,
*                      i2c_PE_INT    - parity error,
*                      i2c_RX_INT    - receive buffer ready,
*                      i2c_TX_INT    - transmit buffer ready,
*                      i2c_WAKE_INT  - wakeup,
*                      i2c_BREAK_INT - break detect
*/
void i2cDisableNotification(i2cBASE_t *i2c, uint32 flags)
{
    uint32 index = i2c == i2cREG1 ? 0U : 1U;

/* USER CODE BEGIN (33) */
/* USER CODE END */

    g_i2cTransfer_t[index].mode &= ~(flags & I2C_TX_INT);
    i2cREG1->IMR               = (flags & (~I2C_TX_INT));
}


/* USER CODE BEGIN (34) */
/* USER CODE END */

/** @fn void i2cInterrupt(void)
*   @brief Interrupt for I2C
*/
#pragma CODE_STATE(i2cInterrupt, 32)
#pragma INTERRUPT(i2cInterrupt, IRQ)

void i2cInterrupt(void)
{
    uint32 vec = (i2cREG1->IVR & 0x00000007U);

/* USER CODE BEGIN (35) */
/* USER CODE END */

    switch (vec)
    {
    case 1U:
/* USER CODE BEGIN (36) */
/* USER CODE END */
        i2cNotification(i2cREG1, I2C_AL_INT);
        break;
    case 2U:
/* USER CODE BEGIN (37) */
/* USER CODE END */
        i2cNotification(i2cREG1, I2C_NACK_INT);
        break;
    case 3U:
/* USER CODE BEGIN (38) */
/* USER CODE END */
        i2cNotification(i2cREG1, I2C_ARDY_INT);
        break;
    case 4U:
/* USER CODE BEGIN (39) */
/* USER CODE END */
        /* receive */
        {   uint32 byte = i2cREG1->DRR;

            if (g_i2cTransfer_t[0U].length > 0U)
            {
                *g_i2cTransfer_t[0U].data++ = byte;
                g_i2cTransfer_t[0U].length--;
                if (g_i2cTransfer_t[0U].length == 0U)
                {
                    i2cNotification(i2cREG1, I2C_RX_INT);
                }
            }
        }
        break;
    case 5U:
/* USER CODE BEGIN (40) */
/* USER CODE END */
        /* transmit */
    	if (g_i2cTransfer_t[0U].length) {
			if (--g_i2cTransfer_t[0U].length > 0U)
			{
				i2cREG1->DXR = *g_i2cTransfer_t[0U].data++;
			}
			else
			{
				i2cREG1->STR = I2C_TX_INT;
				i2cNotification(i2cREG1, I2C_TX_INT);
			}
    	} else {
			i2cREG1->STR = I2C_TX_INT;
    	}
        break;


    case 6U:
/* USER CODE BEGIN (41) */
/* USER CODE END */
        /* transmit */	
        i2cNotification(i2cREG1, I2C_SCD_INT);
        break;

    case 7U:
/* USER CODE BEGIN (42) */
/* USER CODE END */
        i2cNotification(i2cREG1, I2C_AAS_INT);
        break;

    default:
/* USER CODE BEGIN (43) */
/* USER CODE END */
        /* phantom interrupt, clear flags and return */
        i2cREG1->STR = 0x000007FFU;
        break;
    }
/* USER CODE BEGIN (44) */
/* USER CODE END */
}

