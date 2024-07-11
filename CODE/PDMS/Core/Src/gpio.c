/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOE);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOF);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOG);
  LL_PWR_EnableVddIO2();

  /**/
  LL_GPIO_ResetOutputPin(GPIOE, STATUS_LED_Pin|VOLTAGE_MUX_SEL4_Pin|VOLTAGE_MUX_SEL3_Pin|VOLTAGE_MUX_SEL2_Pin
                          |VOLTAGE_MUX_SEL1_Pin|CAN_TERM1_Pin|CAN_TERM2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOF, LP_SIG1_Pin|LP_SIG2_Pin|LP_SIG3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOD, PWM_SIG8_Pin|PWM_SIG7_Pin|PWM_SIG6_Pin|PWM_SIG5_Pin
                          |LP_CSN2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOG, BUZZ_CTRL_Pin|LP_CSN1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(GPIOC, PWM_SIG4_Pin|PWM_SIG3_Pin|PWM_SIG2_Pin|PWM_SIG1_Pin);

  /**/
  GPIO_InitStruct.Pin = STATUS_LED_Pin|VOLTAGE_MUX_SEL4_Pin|VOLTAGE_MUX_SEL3_Pin|VOLTAGE_MUX_SEL2_Pin
                          |VOLTAGE_MUX_SEL1_Pin|CAN_TERM1_Pin|CAN_TERM2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LP_SIG1_Pin|LP_SIG2_Pin|LP_SIG3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LP_SIG4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LP_SIG4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = PWM_SIG8_Pin|PWM_SIG7_Pin|PWM_SIG6_Pin|PWM_SIG5_Pin
                          |LP_CSN2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = BUZZ_CTRL_Pin|LP_CSN1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = PWM_SIG4_Pin|PWM_SIG3_Pin|PWM_SIG2_Pin|PWM_SIG1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = SAFETY_IN_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(SAFETY_IN_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
