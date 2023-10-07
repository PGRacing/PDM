/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STATUS_LED_Pin GPIO_PIN_2
#define STATUS_LED_GPIO_Port GPIOE
#define SENS_OUT8_Pin GPIO_PIN_5
#define SENS_OUT8_GPIO_Port GPIOF
#define SENS_OUT7_Pin GPIO_PIN_10
#define SENS_OUT7_GPIO_Port GPIOF
#define SENS_OUT6_Pin GPIO_PIN_0
#define SENS_OUT6_GPIO_Port GPIOC
#define SENS_OUT5_Pin GPIO_PIN_1
#define SENS_OUT5_GPIO_Port GPIOC
#define HP_MOS_ADC_Pin GPIO_PIN_2
#define HP_MOS_ADC_GPIO_Port GPIOC
#define SENS_OUT4_Pin GPIO_PIN_3
#define SENS_OUT4_GPIO_Port GPIOC
#define SENS_OUT3_Pin GPIO_PIN_0
#define SENS_OUT3_GPIO_Port GPIOA
#define SENS_OUT2_Pin GPIO_PIN_1
#define SENS_OUT2_GPIO_Port GPIOA
#define SENS_OUT1_Pin GPIO_PIN_2
#define SENS_OUT1_GPIO_Port GPIOA
#define IO_CONN8_Pin GPIO_PIN_3
#define IO_CONN8_GPIO_Port GPIOA
#define IO_CONN7_Pin GPIO_PIN_4
#define IO_CONN7_GPIO_Port GPIOA
#define IO_CONN6_Pin GPIO_PIN_5
#define IO_CONN6_GPIO_Port GPIOA
#define IO_CONN5_Pin GPIO_PIN_6
#define IO_CONN5_GPIO_Port GPIOA
#define IO_CONN4_Pin GPIO_PIN_7
#define IO_CONN4_GPIO_Port GPIOA
#define IO_CONN3_Pin GPIO_PIN_4
#define IO_CONN3_GPIO_Port GPIOC
#define IO_CONN2_Pin GPIO_PIN_5
#define IO_CONN2_GPIO_Port GPIOC
#define IO_CONN1_Pin GPIO_PIN_0
#define IO_CONN1_GPIO_Port GPIOB
#define PWM_SIG1_Pin GPIO_PIN_9
#define PWM_SIG1_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
