#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "bsp_out.h"

typedef struct _T_BSP_OUT_CFG
{
    const T_IO     io;
    TIM_TypeDef* tim;
    uint8_t ch;
}T_BSP_OUT_CFG;

static const T_BSP_OUT_CFG bspOutsCfg[OUT_MAX] = 
{
    [OUT_ID_1] = 
    {
        .io = {PWM_SIG1_GPIO_Port, PWM_SIG1_Pin},
        .tim = TIM3,
        .ch = 4,
    },
    [OUT_ID_2] = 
    {
        .io = {PWM_SIG2_GPIO_Port, PWM_SIG2_Pin},
        .tim = TIM3,
        .ch = 3,
    },
    [OUT_ID_3] = 
    {
        .io = {PWM_SIG3_GPIO_Port, PWM_SIG3_Pin},
        .tim = TIM3,
        .ch = 2,
    },
    [OUT_ID_4] = 
    {
        .io = {PWM_SIG4_GPIO_Port, PWM_SIG4_Pin},
        .tim = TIM3,
        .ch = 1,
    },
    [OUT_ID_5] = 
    {
        .io = {PWM_SIG5_GPIO_Port, PWM_SIG5_Pin},
        .tim = TIM4,
        .ch = 4,
    },
    [OUT_ID_6] = 
    {
        .io = {PWM_SIG6_GPIO_Port, PWM_SIG6_Pin},
        .tim = TIM4,
        .ch = 3,
    },
    [OUT_ID_7] = 
    {
        .io = {PWM_SIG7_GPIO_Port, PWM_SIG7_Pin},
        .tim = TIM4,
        .ch = 2,
    },
    [OUT_ID_8] = 
    {
        .io = {PWM_SIG8_GPIO_Port, PWM_SIG8_Pin},
        .tim = TIM4,
        .ch = 1,
    },
};

static inline T_IO BSP_OUT_GetIO(T_OUT_ID id)
{
    return bspOutsCfg[id].io;
}

void BSP_OUT_SetMode(T_OUT_ID id, T_OUT_MODE mode)
{
    ASSERT( id < OUT_MAX);
    T_IO io = BSP_OUT_GetIO(id);
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

void BSP_OUT_SetStdState(T_OUT_ID id, bool state)
{
    ASSERT( id < OUT_MAX);
    T_IO io = BSP_OUT_GetIO(id);
    
    if(state)
    {
        LL_GPIO_SetOutputPin(io.port, io.pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(io.port, io.pin);
    }
}

void BSP_OUT_SetBatchState(T_OUT_ID id, T_OUT_ID batchId, bool state)
{
    ASSERT( id < OUT_MAX);
    ASSERT( batchId < OUT_MAX);
    T_IO io = BSP_OUT_GetIO(id);
    T_IO batchIo = BSP_OUT_GetIO(batchId);

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

bool BSP_OUT_IsBatchPossible(T_OUT_ID id, T_OUT_ID batchId)
{
    ASSERT( id < OUT_MAX);
    ASSERT( batchId < OUT_MAX);
    
    return (BSP_OUT_GetIO(id).port == BSP_OUT_GetIO(batchId).port);
}

void BSP_OUT_InitPWM(T_OUT_ID id)
{

}

void BSP_OUT_DeInitPWM(T_OUT_ID id)
{
}

void BSP_OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty)
{
}

void BSP_OUT_Init(T_IO io)
{

}