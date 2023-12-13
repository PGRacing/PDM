#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "bsp_out.h"

void BSP_OUT_SetMode(T_IO io, T_OUT_MODE mode)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    switch(mode)
    {
        case OUT_MODE_UNUSED:
            LL_GPIO_ResetOutputPin(io.port, io.pin);
            GPIO_InitStruct.Pin = io.pin;
            GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
            GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            LL_GPIO_Init(io.port, &GPIO_InitStruct);
        break;
        
        case OUT_MODE_STD:
            LL_GPIO_ResetOutputPin(io.port, io.pin);
            GPIO_InitStruct.Pin = io.pin;
            GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
            GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            LL_GPIO_Init(io.port, &GPIO_InitStruct);
        break;

        case OUT_MODE_PWM:
            LL_GPIO_ResetOutputPin(io.port, io.pin);
        break;

        default:
        break;
    }
}

void BSP_OUT_SetStdState(T_IO io, bool state)
{
    if(state)
    {
        LL_GPIO_SetOutputPin(io.port, io.pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(io.port, io.pin);
    }
}

void BSP_OUT_SetBatchState(T_IO io, T_IO batchIo, bool state)
{
    if( io.port == batchIo.port )
    {
        if(state)
        {
            LL_GPIO_SetOutputPin(io.port, (io.pin | batchIo.pin));
        }
        else
        {
            LL_GPIO_ResetOutputPin(io.port, (io.pin | batchIo.pin));
        }
    }
}

void BSP_OUT_InitPWM(T_IO io)
{
}

void BSP_OUT_DeInitPWM(T_IO io)
{
}

void BSP_OUT_SetDutyPWM(T_IO io, uint8_t duty)
{
}

void BSP_OUT_Init(T_IO io)
{

}