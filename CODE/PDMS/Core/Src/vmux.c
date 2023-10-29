#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "llgpio.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "adc.h"
#include "cmsis_os2.h"

#define VMUX_SELECTOR_COUNT 3
#define VMUX_INPUT_COUNT 8
#define VMUX_SELECTOR_MAX_VAL 8

volatile uint32_t VMUX_AdcValue[VMUX_INPUT_COUNT];

static T_IO VMUX_SelectorConfig[VMUX_SELECTOR_COUNT] = 
{
    {.gpio = VOLTAGE_MUX_SEL1_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL1_Pin},
    {.gpio = VOLTAGE_MUX_SEL2_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL2_Pin},
    {.gpio = VOLTAGE_MUX_SEL3_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL3_Pin},
};

void VMUX_Init()
{
    // Set multiplexer selector pins to output mode
    LLGPIO_SetMode(VMUX_SelectorConfig[0], LL_GPIO_MODE_OUTPUT);
    LLGPIO_SetMode(VMUX_SelectorConfig[1], LL_GPIO_MODE_OUTPUT);
    LLGPIO_SetMode(VMUX_SelectorConfig[2], LL_GPIO_MODE_OUTPUT);
}

static void VMUX_SelectInput( uint8_t selector )
{
    ASSERT( selector >= 0 );
    ASSERT( selector < VMUX_SELECTOR_MAX_VAL );
    
    LLGPIO_SetStdState(VMUX_SelectorConfig[0], 0 != (selector & 0x01));
    LLGPIO_SetStdState(VMUX_SelectorConfig[1], 0 != (selector & 0x02));
    LLGPIO_SetStdState(VMUX_SelectorConfig[2], 0 != (selector & 0x04));
}

static void VMUX_GetAllPooling()
{
    for( uint8_t sel = 0; sel < VMUX_SELECTOR_MAX_VAL; sel++)
    {
        VMUX_SelectInput( sel );
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
        osDelay(50);
    }
    /* USER CODE END vmuxTaskStart */
}