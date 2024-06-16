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
#include "bsp_out.h"
#include "cmsis_os2.h"

#define VMUX_SELECTOR_COUNT 4
#define VMUX_INPUT_COUNT 16
#define VMUX_SELECTOR_MAX_VAL 16
#define VMUX_SELECTOR_PORT GPIOE

// This assumes that voltage divider is same on all channels
#define VMUX_STORE_VOLTAGE
#define VMUX_USED_DIVIDER (2.32 / 12.32)
//#define VMUX_USED_DIVIDER_INV 5.310344827586207
#define VMUX_USED_DIVIDER_INV 5.44853224

#define VMUX_ADC_12BIT_MAX_VALUE 4096

#define VMUX_GET_VOLTAGE_MV(X) (X * VDD_VALUE / VMUX_ADC_12BIT_MAX_VALUE * VMUX_USED_DIVIDER_INV)

volatile uint32_t VMUX_BattVoltage = 13800;

volatile uint32_t VMUX_Value[VMUX_INPUT_COUNT] = {0};

static const uint8_t VMUX_ReadOrder[VMUX_INPUT_COUNT] = {2, 4, 3, 5, 6, 7, 0, 1, 8, 9, 10, 11, 12, 13, 14, 15};

static T_IO VMUX_SelectorConfig[VMUX_SELECTOR_COUNT] = 
{
    {.port = VOLTAGE_MUX_SEL1_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL1_Pin},
    {.port = VOLTAGE_MUX_SEL2_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL2_Pin},
    {.port = VOLTAGE_MUX_SEL3_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL3_Pin},
    {.port = VOLTAGE_MUX_SEL4_GPIO_Port,
     .pin  = VOLTAGE_MUX_SEL4_Pin
    }
};

void VMUX_Init()
{
    // Set multiplexer selector pins to output mode
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[0].port, VMUX_SelectorConfig[0].pin);
    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[1].port, VMUX_SelectorConfig[1].pin);
    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[2].port, VMUX_SelectorConfig[2].pin);
    LL_GPIO_ResetOutputPin(VMUX_SelectorConfig[3].port, VMUX_SelectorConfig[3].pin);

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

    GPIO_InitStruct.Pin = VMUX_SelectorConfig[3].pin;
    LL_GPIO_Init(VMUX_SelectorConfig[3].port, &GPIO_InitStruct);
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

void VMUX_SelectMuxAdcChannel (void)
{
	ADC_ChannelConfTypeDef sConfig = {0};
    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
     */
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    sConfig.Offset = 0;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void  VMUX_SelectBatteryAdcChannel()
{
    ADC_ChannelConfTypeDef sConfig = {0};
    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
     */
    sConfig.Channel = ADC_CHANNEL_13;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    sConfig.Offset = 0;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

static void VMUX_ReadBattVoltage()
{
    VMUX_SelectBatteryAdcChannel();
    HAL_ADC_Start(&hadc3);
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);
    if(HAL_ADC_PollForConversion(&hadc3, 50) == HAL_OK)
    {   
        #ifdef VMUX_STORE_VOLTAGE
            VMUX_BattVoltage = VMUX_GET_VOLTAGE_MV(HAL_ADC_GetValue(&hadc3));
        #endif
    }       
    HAL_ADC_Stop(&hadc3);
}

static void VMUX_GetAllPooling()
{
    VMUX_SelectMuxAdcChannel();
    HAL_ADC_Start(&hadc3);
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);

    for( uint8_t sel = 0; sel < VMUX_SELECTOR_MAX_VAL; sel++)
    {
        VMUX_SelectInput( VMUX_ReadOrder[sel] );

        if(HAL_ADC_PollForConversion(&hadc3, 50) == HAL_OK)
        {   
            #ifdef VMUX_STORE_VOLTAGE
                VMUX_Value[sel] = VMUX_GET_VOLTAGE_MV(HAL_ADC_GetValue(&hadc3));
            #else
                VMUX_Value[sel] = HAL_ADC_GetValue(&hadc3);
            #endif
        }       
    }

    HAL_ADC_Stop(&hadc3);
}

uint32_t VMUX_GetValue(uint8_t index)
{
    ASSERT(index < VMUX_INPUT_COUNT);
    return VMUX_Value[index];
}

uint32_t VMUX_GetBattValue()
{
    return VMUX_BattVoltage;
}

void vmuxTaskStart(void *argument)
{
    /* USER CODE BEGIN vmuxTaskStart */
    VMUX_Init();
    /* Infinite loop */
    for(;;)
    {
        VMUX_ReadBattVoltage();
        VMUX_GetAllPooling();
        osDelay(pdMS_TO_TICKS(5));
    }
    /* USER CODE END vmuxTaskStart */
}