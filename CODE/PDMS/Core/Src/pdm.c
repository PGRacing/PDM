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
#include "buzzer.h"
#include "task.h"
#include "tim.h"
#include "semphr.h"
#include "pdm.h"
// Main PDM file

void pdmTaskStart(void *argument)
{   
    // Initialization sequence
    WS2812B_Init(&htim2, TIM_CHANNEL_4);
    WS2812B_StartupAction();
    
    BUZZER_TurnOn();
    osDelay(300);
    BUZZER_TurnOff();

    // Initalize SPOC2 device (move to OUT_Init later)
    SPOC2_Init();


    // Set first channel into PWM mode as example
    OUT_ChangeMode( OUT_ID_1, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_2, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_3, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_4, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_5, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_6, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_7, OUT_MODE_PWM);
    OUT_ChangeMode( OUT_ID_8, OUT_MODE_PWM);
    BSP_OUT_SetDutyPWM(OUT_ID_1, 10);
    BSP_OUT_SetDutyPWM(OUT_ID_2, 20);
    BSP_OUT_SetDutyPWM(OUT_ID_3, 30);
    BSP_OUT_SetDutyPWM(OUT_ID_4, 40);
    BSP_OUT_SetDutyPWM(OUT_ID_5, 50);
    BSP_OUT_SetDutyPWM(OUT_ID_6, 60);
    BSP_OUT_SetDutyPWM(OUT_ID_7, 70);
    BSP_OUT_SetDutyPWM(OUT_ID_8, 80);

    for(;;)
    {
        // It should return false
        OUT_ToggleState(OUT_ID_1);
        osDelay(2000);
    }

}
