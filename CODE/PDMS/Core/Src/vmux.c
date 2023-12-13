#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_ll_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "adc.h"
#include "cmsis_os2.h"

#define VMUX_SELECTOR_COUNT 3
#define VMUX_INPUT_COUNT 8
#define VMUX_SELECTOR_MAX_VAL 8
#define VMUX_SELECTOR_PORT GPIOE

volatile uint32_t VMUX_AdcValue[VMUX_INPUT_COUNT];

static const uint8_t VMUX_ReadOrder[VMUX_INPUT_COUNT] = {2, 1, 0, 3, 5, 7, 6, 4};

static T_IO VMUX_SelectorConfig[VMUX_SELECTOR_COUNT] = 
{
    {.port = VOLTAGE_MUX_SEL1_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL1_Pin},
    {.port = VOLTAGE_MUX_SEL2_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL2_Pin},
    {.port = VOLTAGE_MUX_SEL3_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL3_Pin},
};

void VMUX_Init()
{
    // Set multiplexer selector pins to output mode
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[0].port, VMUX_SelectorConfig[0].pin);
    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[1].port, VMUX_SelectorConfig[1].pin);
    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[2].port, VMUX_SelectorConfig[2].pin);

    GPIO_InitStruct.Pin = VMUX_SelectorConfig[0].pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(VMUX_SelectorConfig[0].port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = VMUX_SelectorConfig[1].pin;
    LL_GPIO_Init(VMUX_SelectorConfig[1].port, &GPIO_InitStruct);

    
    GPIO_InitStruct.Pin = VMUX_SelectorConfig[2].pin;
    LL_GPIO_Init(VMUX_SelectorConfig[2].port, &GPIO_InitStruct);
}

static void VMUX_SelectInput( uint8_t selector )
{
    ASSERT( selector >= 0 );
    ASSERT( selector < VMUX_SELECTOR_MAX_VAL );

    uint32_t setMask = 0;
    uint32_t resetMask = 0;
    
    for( uint8_t i = 0, j = 0x01; i < VMUX_SELECTOR_COUNT; i++, j *= 2)
    {
        if(0 != (selector & j))
        {
            setMask |= VMUX_SelectorConfig[i].pin;
        }
        else
        {
            resetMask |= VMUX_SelectorConfig[i].pin;
        }
    }
    
    LL_GPIO_SetOutputPin(VMUX_SELECTOR_PORT, setMask);
    LL_GPIO_ResetOutputPin(VMUX_SELECTOR_PORT, resetMask);
}

static void VMUX_GetAllPooling()
{
    for( uint8_t sel = 0; sel < VMUX_SELECTOR_MAX_VAL; sel++)
    {

        VMUX_SelectInput( VMUX_ReadOrder[sel] );
        HAL_ADC_Start(&hadc3);
        if(HAL_ADC_PollForConversion(&hadc3, 20) == HAL_OK)
        {
            VMUX_AdcValue[sel] = HAL_ADC_GetValue(&hadc3);
        }
    }
}

void vmuxTaskStart(void *argument)
{
    /* USER CODE BEGIN vmuxTaskStart */
    VMUX_Init();
    /* Infinite loop */
    for(;;)
    {
        VMUX_GetAllPooling();
        osDelay(10);
    }
    /* USER CODE END vmuxTaskStart */
}