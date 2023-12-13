#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "logic.h"
#include "out.h"
#include "task.h"
#include "tim.h"
#include "semphr.h"
#include "pdm.h"
// Main PDM file

// void pdmTaskStart(void *argument)
// {

//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_1), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_2), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_3), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_4), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_5), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_6), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_7), OUT_MODE_STD);
//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_8), OUT_MODE_STD);

//   for (;;)
//   {
//     // for(uint8_t id = 0; id < 8; id++)
//     // // {
//     // //     OUT_ToggleState(OUT_GetPtr(id));
//     // // }
//     // // osDelay(2200);
//     vTaskSuspend(NULL);
//   }
// }
