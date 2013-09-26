/** @file sys_main.c 
*   @brief Application main file
*   @date 29.May.2013
*   @version 03.05.02
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* (c) Texas Instruments 2009-2013, All rights reserved. */

/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
extern void board_init_scheduler_not_running(void);
extern int app_main(void);
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
/* USER CODE END */

void main(void)
{
/* USER CODE BEGIN (3) */
	board_init_scheduler_not_running();

	app_main();
/* USER CODE END */
}

/* USER CODE BEGIN (4) */
/* USER CODE END */
