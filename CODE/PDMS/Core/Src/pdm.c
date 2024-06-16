#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_it.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "ws2812b.h"
#include "logic.h"
#include "out.h"
#include "buzzer.h"
#include "task.h"
#include "tim.h"
#include "semphr.h"
#include "pdm.h"
#include "telemetry.h"
// Main PDM file

T_PDM_SYS_STATUS PDM_GetSysStatus()
{
    // TODO Add status handlers CAN status etc...
    return PDM_SYS_STATUS_OK;
}

static T_OUT_MODE PDM_OutModeInitTable[OUT_ID_MAX] = 
{
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
};

static void PDM_OutConfig()
{
    for(uint8_t i = 0; i < OUT_ID_MAX; i++)
    {
        OUT_ChangeMode( i, PDM_OutModeInitTable[i] );
    }
}

static void PDM_Init()
{
    // SPOC2 External outputs init (all outputs disabled)
    SPOC2_Init();

    // Initialize output module (set correct mode and state)
    PDM_OutConfig();

    // Initialize ARGB'S
    WS2812B_Init(&htim2, TIM_CHANNEL_4);
    
    // Do a startup led action
    WS2812B_StartupAction();
}

void pdmTaskStart(void *argument)
{   
    // Initialization sequence 
    PDM_Init();
    
    for(;;)
    {
        bool* logicResults = LOGIC_Evaluate();
        for(uint8_t i = 0; i < OUT_ID_MAX; i++)
        {
            OUT_SetState(i, logicResults[i]);
        }
        osDelay(pdMS_TO_TICKS(1));
    }
}
