/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for statusTask */
osThreadId_t statusTaskHandle;
const osThreadAttr_t statusTask_attributes = {
  .name = "statusTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for can1Task */
osThreadId_t can1TaskHandle;
const osThreadAttr_t can1Task_attributes = {
  .name = "can1Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for can2Task */
osThreadId_t can2TaskHandle;
const osThreadAttr_t can2Task_attributes = {
  .name = "can2Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for adcTask */
osThreadId_t adcTaskHandle;
const osThreadAttr_t adcTask_attributes = {
  .name = "adcTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for testTask */
osThreadId_t testTaskHandle;
const osThreadAttr_t testTask_attributes = {
  .name = "testTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for vmuxTask */
osThreadId_t vmuxTaskHandle;
const osThreadAttr_t vmuxTask_attributes = {
  .name = "vmuxTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for pdmTask */
osThreadId_t pdmTaskHandle;
const osThreadAttr_t pdmTask_attributes = {
  .name = "pdmTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for spoc2CurrTask */
osThreadId_t spoc2CurrTaskHandle;
const osThreadAttr_t spoc2CurrTask_attributes = {
  .name = "spoc2CurrTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void statusTaskStart(void *argument);
extern void can1TaskStart(void *argument);
extern void can2TaskStart(void *argument);
extern void adcTaskStart(void *argument);
extern void testTaskEntry(void *argument);
extern void vmuxTaskStart(void *argument);
extern void pdmTaskStart(void *argument);
extern void spoc2CurrTaskStart(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of statusTask */
  statusTaskHandle = osThreadNew(statusTaskStart, NULL, &statusTask_attributes);

  /* creation of can1Task */
  can1TaskHandle = osThreadNew(can1TaskStart, NULL, &can1Task_attributes);

  /* creation of can2Task */
  can2TaskHandle = osThreadNew(can2TaskStart, NULL, &can2Task_attributes);

  /* creation of adcTask */
  adcTaskHandle = osThreadNew(adcTaskStart, NULL, &adcTask_attributes);

  /* creation of testTask */
  testTaskHandle = osThreadNew(testTaskEntry, NULL, &testTask_attributes);

  /* creation of vmuxTask */
  vmuxTaskHandle = osThreadNew(vmuxTaskStart, NULL, &vmuxTask_attributes);

  /* creation of pdmTask */
  pdmTaskHandle = osThreadNew(pdmTaskStart, NULL, &pdmTask_attributes);

  /* creation of spoc2CurrTask */
  spoc2CurrTaskHandle = osThreadNew(spoc2CurrTaskStart, NULL, &spoc2CurrTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(pdMS_TO_TICKS(100));
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_statusTaskStart */
/**
* @brief Function implementing the statusTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_statusTaskStart */
void statusTaskStart(void *argument)
{
  /* USER CODE BEGIN statusTaskStart */
  /* Infinite loop */
    for(;;)
    {
        //HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
        osDelay(500);
    }
  /* USER CODE END statusTaskStart */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

