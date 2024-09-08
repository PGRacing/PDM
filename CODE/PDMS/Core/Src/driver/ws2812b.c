#include "ws2812b.h"
#include <stm32l4xx_hal_tim.h>
#include <string.h>
#include <math.h>
#include "cmsis_os2.h"
#include "tim.h"
#include "out.h"
#include "typedefs.h"
#include "FreeRTOS.h"
#include "pdm.h"

/* This values work when full cycle of timer is set for 1.25us
 * For eg. internal clock = 16Mhz  prescaler = 0 counter_period = 20-1
 * Warning! For WS2812B-2020 Timings are slightly diffrent from WS2812B-5050 
 * */

#define LED_NUM 18
#define RESET_SIG_SIZE 400
// 24 and 48
#define ON_STATE_BIT 10
#define OFF_STATE_BIT 5
#define DATA_SIZE 24

rgb_t red = {0x3F, 0x00, 0x00};
rgb_t green = {0x00, 0x3F, 0x00};
rgb_t blue = {0x00, 0x00, 0x3F};
rgb_t clear = {0x00, 0x00, 0x00};
rgb_t rcpergol = {219, 82, 15};

const uint8_t gamma8[] =
{
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
        10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
        17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
        25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
        37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
        51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
        69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
        90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
        115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
        144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
        177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
        215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

struct
{
    TIM_HandleTypeDef* htim;
    uint32_t pwm_channel;
}ws2812b_conf;

uint8_t buff[RESET_SIG_SIZE + LED_NUM * DATA_SIZE + 1];
uint8_t dpbuff[RESET_SIG_SIZE + LED_NUM * DATA_SIZE + 1];

static void WS2812B_SetByteHelper(uint32_t pos, uint8_t value)
{

    for(uint8_t i = 0; i < 8; i++)
    {
        buff[pos + i] = (value & 0x80) ? ON_STATE_BIT : OFF_STATE_BIT;
        value <<= 1;
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{   
    if(htim == &htim2)
    {
        HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_4);
    }
}


void WS2812B_Init(TIM_HandleTypeDef* htim, uint32_t Channel)
{
    ws2812b_conf.htim = htim;
    ws2812b_conf.pwm_channel = Channel;

    /* PWM duty = 0% - RESET */
    memset(buff, 0x00, RESET_SIG_SIZE);

    /* PWM duty = 32% - OFF_STATE_BIT */
    memset(buff + RESET_SIG_SIZE, OFF_STATE_BIT, ARRAY_COUNT(buff) - RESET_SIG_SIZE);

    buff[40] = ON_STATE_BIT;
    /* PWM duty = 100% - EOT */
    buff[ARRAY_COUNT(buff) - 1] = 100;

    HAL_TIM_Base_Start(htim);
}

void WS2812B_Flush(void)
{
    memset(buff, 0x00, RESET_SIG_SIZE);
    buff[ARRAY_COUNT(buff) - 1] = 100;
    HAL_TIM_PWM_Start_DMA(ws2812b_conf.htim, ws2812b_conf.pwm_channel, (const uint32_t*)&buff, ARRAY_COUNT(buff));
    memcpy(dpbuff, buff, sizeof(buff));
}

// Flush only if update needed
void WS2812B_FlushIf(void)
{
    if(memcmp(dpbuff, buff, sizeof(buff)) != 0)
    {
        WS2812B_Flush();
    }
}


void WS2812B_SetSingle(uint16_t led_index, rgb_t color)
{
    vPortEnterCritical();
    if (led_index < LED_NUM)
    {
        WS2812B_SetByteHelper(RESET_SIG_SIZE + DATA_SIZE * led_index, gamma8[color.g]);
        WS2812B_SetByteHelper(RESET_SIG_SIZE + DATA_SIZE * led_index + 8,  gamma8[color.r]);
        WS2812B_SetByteHelper(RESET_SIG_SIZE + DATA_SIZE * led_index + 16, gamma8[color.b]);
    }
    vPortExitCritical();
}

void WS2812B_Clear(void)
{
    for(uint8_t i =0; i < LED_NUM; i++)
    {
        WS2812B_SetSingle(i, clear);
    }
}

static rgb_t WS2812B_ChangeColorLumin(rgb_t color, uint8_t lumin)
{
    // lumin value from 0 - 100 of color luminosity
    color.g = (uint16_t)color.g * lumin / 100;
    color.r = (uint16_t)color.r * lumin / 100;
    color.b = (uint16_t)color.b * lumin / 100;
    return color;
}

void WS2812B_StartupAction()
{
    WS2812B_Clear();

    for(uint8_t j = 0; j < LED_NUM; j++)
    {
        WS2812B_SetSingle(j, WS2812B_ChangeColorLumin(rcpergol, 50));
        WS2812B_Flush();
        osDelay(50);
    }

    osDelay(500);
    WS2812B_Clear();
    WS2812B_Flush();
}


#define WS2812B_STAT_LED1 17
#define WS2812B_STAT_LED2 18
void WS2812B_EvaluateLeds()
{
    // SETUP LEDS

    for (uint8_t id = 0; id < OUT_ID_MAX; id++)
    {
        
        //T_OUT_TYPE t = OUT_GetType(id);

        // T_OUT_STATE s = OUT_DIAG_GetState(id);
        // switch (s)
        // {
        // case OUT_STATE_ON:
        //     WS2812B_SetSingle(id, green);
        //     break;

        // case OUT_STATE_OFF:
        //     WS2812B_SetSingle(id, clear);
        //     break;

        // case OUT_STATE_ERR_LATCH:
        //     WS2812B_SetSingle(id, red);
        //     break;

        // default:
        //     break;
        // }

        // Better option based on Status
        // if(t == OUT_TYPE_BTS500)
        // {
        //     T_OUT_STATUS s = OUT_DIAG_GetStatus(id);
        //     switch (s)
        //     {
        //     case OUT_STATUS_NORMAL_OFF:
        //         WS2812B_SetSingle(id, clear);
        //         break;
        //     case OUT_STATUS_NORMAL_ON:
        //         WS2812B_SetSingle(id, green);
        //         break;
        //     case OUT_STATUS_ERR_LATCH:
        //         WS2812B_SetSingle(id, red);
        //         break;
        //     case OUT_STATUS_OPEN_LOAD:
        //     case OUT_STATUS_HARD_OC_OR_OT:
        //     case OUT_STATUS_S_AND_H_OC:
        //     case OUT_STATUS_SOFT_OC:
        //     case OUT_STATUS_SHORT_TO_VSS:
        //         WS2812B_SetSingle(id, rcpergol);
        //         break;

        //     default:
        //         WS2812B_SetSingle(id, clear);
        //         break;
        //     }
        // }
        // else if(t == OUT_TYPE_SPOC2)
        // {   
        //     T_OUT_STATE s = OUT_DIAG_GetState(id);
        //     switch (s)
        //     {
        //     case OUT_STATE_ON:
        //         WS2812B_SetSingle(id, green);
        //         break;

        //     case OUT_STATE_OFF:
        //         WS2812B_SetSingle(id, clear);
        //         break;
        //     default:
        //         break;
        //     }
        // }
    }

    T_PDM_SYS_STATUS sysStat = PDM_GetSysStatus();
    switch (sysStat)
    {
    case PDM_SYS_STATUS_OK:
        WS2812B_SetSingle(WS2812B_STAT_LED1, green);
        break;
    case PDM_SYS_STATUS_ERROR: 
        WS2812B_SetSingle(WS2812B_STAT_LED1, blue);
        break;
    default:
        break;
    }
}

// TODO Move to another module
void argbTaskStart(void *argument)
{
    /* USER CODE BEGIN telemTaskStart */
    // Initialize ARGB'S
    WS2812B_Init(&htim2, TIM_CHANNEL_4);
    // Do a startup led action
    WS2812B_StartupAction();
    /* Infinite loop */
    for (;;)
    {
        osDelay(pdMS_TO_TICKS(100));
        WS2812B_EvaluateLeds();
        WS2812B_FlushIf();
    }
    /* USER CODE END telemTaskStart */
}

void WS2812_Test()
{
    // for(int i = 0; i < 1000; i++)
    // {
    //     float r = 50 * (1.0f + sin(i / 100.0f));
    //     float g = 50 * (1.0f + sin(1.5f * i / 100.0f));
    //     float b = 50 * (1.0f + sin(2.0f * i / 100.0f));
    //     rgb_t constructed_rgb = {r, g, b};

    //     for(uint8_t j = 0; j < LED_NUM; j++)
    //     {
    //         WS2812B_SetSingle(j,constructed_rgb);
    //     }
    //     osDelay(10);
    //     WS2812B_Flush();
    // }
}

///
/// TODO [MAJOR REWORK] Change timing of signals to have more of head-room between limits, and add signal level shifter
///