/**
  ******************************************************************************
  * @file           : demo.h
  * @brief          : Header for polling tag detect
  ******************************************************************************
  * @attention
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DEMO_H
#define DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define PLATFORM_USER_BUTTON_PORT   BUTTON_0_GPIO_PORT
#define PLATFORM_USER_BUTTON_PIN    BUTTON_0_GPIO_PIN  
/* Exported functions ------------------------------------------------------- */
/** @defgroup PTD_Demo_Exported_Functions
  *  @{
  */
bool demoIni(void);
void demoCycle(void);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_H */

