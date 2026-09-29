/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  main() calls the target system initialization, then calls the example entry point.
  ******************************************************************************
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "mx_basic_stdio_app.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* @user: choose the number of process loops here */
#define EXAMPLE_LOOP_COUNT 200U

/* @user: configure the delay in milliseconds between 2 loop rounds */
#define EXAMPLE_LOOP_DELAY_MS 100U

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
app_status_t ExecStatus = EXEC_STATUS_UNKNOWN; /* application status */

/* Private functions prototype -----------------------------------------------*/
static void error_handler(void);

/**
  * brief:  The application entry point.
  * retval: none but we specify int to comply with C99 standard
  */
int main(void)
{
  /**
    * System Init: this generated code placed in targets folder initializes your system.
    * It calls the initialization (and sets the initial configuration) of the peripherals
    * you have configured with STM32CubeSTUDIO, if you decided to generate and call this
    * code at startup.
    * It also contains the HAL initialization and the initial clock configuration.
  */
  if (mx_system_init() != SYSTEM_OK)
  {
    ExecStatus = EXEC_STATUS_ERROR; /* memorize the error */
  }
  else
  {
    /* Initialize Basic stdio separately, but after system init. */
#if defined(USE_TRACE) && USE_TRACE != 0
    /* Initialize basic_stdio separately, but after system init. */
    mx_basic_stdio_init();
#endif /* defined(USE_TRACE) && USE_TRACE != 0 */

    /**
      * Example execution : this hardware and IDE agnostic code contains the scenario we demonstrate in this example.
      * This is the applicative code showing how to use the peripheral (functionality-wise).
      * You can copy/paste it in your own application,
      * while you need to keep on generating the initialization and configuration code with STM32CubeSTUDIO.
      */
    ExecStatus = app_init();

    /* Run EXAMPLE_LOOP_COUNT times if no error occurs */
    while (ExecStatus != EXEC_STATUS_ERROR)
    {
      ExecStatus = app_process();
    } /* end while */

  } /* end applicative part */

  /* Report the example status */
  error_handler();


  /* This point can not be reached */
  return (0);
} /* end main */

/** ----------------------------------------------------------
  * The functions below are used to report the example status.
  * ----------------------------------------------------------
  */

/**
  * brief:  Error notification
  * retval: None (infinite loop)
  */
static void error_handler(void)
{
  /* Initialize LED_USER */
  /* skip return code as we are in error_handler */

  while (1)
  {
    PRINTF("ERROR HANDLER CASE");
  }
} /* end error_handler */

