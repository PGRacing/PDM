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

#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_dma.h"

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
#define USE_FULL_ASSERT 1
#define STATUS_LED_Pin LL_GPIO_PIN_2
#define STATUS_LED_GPIO_Port GPIOE
#define VOLTAGE_MUX_SEL4_Pin LL_GPIO_PIN_3
#define VOLTAGE_MUX_SEL4_GPIO_Port GPIOE
#define VOLTAGE_MUX_SEL3_Pin LL_GPIO_PIN_4
#define VOLTAGE_MUX_SEL3_GPIO_Port GPIOE
#define VOLTAGE_MUX_SEL2_Pin LL_GPIO_PIN_5
#define VOLTAGE_MUX_SEL2_GPIO_Port GPIOE
#define VOLTAGE_MUX_SEL1_Pin LL_GPIO_PIN_6
#define VOLTAGE_MUX_SEL1_GPIO_Port GPIOE
#define SENS_OUT_LP2_Pin LL_GPIO_PIN_3
#define SENS_OUT_LP2_GPIO_Port GPIOF
#define SENS_OUT_LP1_Pin LL_GPIO_PIN_4
#define SENS_OUT_LP1_GPIO_Port GPIOF
#define VMUX_ADC_Pin LL_GPIO_PIN_5
#define VMUX_ADC_GPIO_Port GPIOF
#define LP_SIG1_Pin LL_GPIO_PIN_6
#define LP_SIG1_GPIO_Port GPIOF
#define LP_SIG2_Pin LL_GPIO_PIN_7
#define LP_SIG2_GPIO_Port GPIOF
#define LP_SIG3_Pin LL_GPIO_PIN_8
#define LP_SIG3_GPIO_Port GPIOF
#define LP_SIG4_Pin LL_GPIO_PIN_9
#define LP_SIG4_GPIO_Port GPIOF
#define SENS_OUT8_Pin LL_GPIO_PIN_0
#define SENS_OUT8_GPIO_Port GPIOC
#define SENS_OUT7_Pin LL_GPIO_PIN_1
#define SENS_OUT7_GPIO_Port GPIOC
#define SENS_OUT6_Pin LL_GPIO_PIN_2
#define SENS_OUT6_GPIO_Port GPIOC
#define SENS_OUT5_Pin LL_GPIO_PIN_3
#define SENS_OUT5_GPIO_Port GPIOC
#define SENS_OUT4_Pin LL_GPIO_PIN_0
#define SENS_OUT4_GPIO_Port GPIOA
#define SENS_OUT3_Pin LL_GPIO_PIN_1
#define SENS_OUT3_GPIO_Port GPIOA
#define SENS_OUT2_Pin LL_GPIO_PIN_2
#define SENS_OUT2_GPIO_Port GPIOA
#define SENS_OUT1_Pin LL_GPIO_PIN_3
#define SENS_OUT1_GPIO_Port GPIOA
#define IO_CONN8_Pin LL_GPIO_PIN_4
#define IO_CONN8_GPIO_Port GPIOA
#define IO_CONN7_Pin LL_GPIO_PIN_5
#define IO_CONN7_GPIO_Port GPIOA
#define IO_CONN6_Pin LL_GPIO_PIN_6
#define IO_CONN6_GPIO_Port GPIOA
#define IO_CONN5_Pin LL_GPIO_PIN_7
#define IO_CONN5_GPIO_Port GPIOA
#define IO_CONN4_Pin LL_GPIO_PIN_4
#define IO_CONN4_GPIO_Port GPIOC
#define IO_CONN3_Pin LL_GPIO_PIN_5
#define IO_CONN3_GPIO_Port GPIOC
#define IO_CONN2_Pin LL_GPIO_PIN_0
#define IO_CONN2_GPIO_Port GPIOB
#define IO_CONN1_Pin LL_GPIO_PIN_1
#define IO_CONN1_GPIO_Port GPIOB
#define LED_DATA_Pin LL_GPIO_PIN_11
#define LED_DATA_GPIO_Port GPIOB
#define PWM_SIG8_Pin LL_GPIO_PIN_12
#define PWM_SIG8_GPIO_Port GPIOD
#define PWM_SIG7_Pin LL_GPIO_PIN_13
#define PWM_SIG7_GPIO_Port GPIOD
#define PWM_SIG6_Pin LL_GPIO_PIN_14
#define PWM_SIG6_GPIO_Port GPIOD
#define PWM_SIG5_Pin LL_GPIO_PIN_15
#define PWM_SIG5_GPIO_Port GPIOD
#define BUZZ_CTRL_Pin LL_GPIO_PIN_7
#define BUZZ_CTRL_GPIO_Port GPIOG
#define PWM_SIG4_Pin LL_GPIO_PIN_6
#define PWM_SIG4_GPIO_Port GPIOC
#define PWM_SIG3_Pin LL_GPIO_PIN_7
#define PWM_SIG3_GPIO_Port GPIOC
#define PWM_SIG2_Pin LL_GPIO_PIN_8
#define PWM_SIG2_GPIO_Port GPIOC
#define PWM_SIG1_Pin LL_GPIO_PIN_9
#define PWM_SIG1_GPIO_Port GPIOC
#define LP_CSN2_Pin LL_GPIO_PIN_7
#define LP_CSN2_GPIO_Port GPIOD
#define LP_CSN1_Pin LL_GPIO_PIN_9
#define LP_CSN1_GPIO_Port GPIOG
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
