#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_ll_tim.h"
#include "typedefs.h"
#include "logger.h"
#include "vmux.h"
#include "bsp_out.h"
#include "adc.h"

//#define LL_TIM_OC_SetCompare(TIM, CNUM, CMP) LL_TIM_OC_SetCompareCH##CNUM(TIM, CMP)

#define BSP_OUT_CURRENT_CLK 80000000

/* Prescaler value used for PWM channels */
#define BSP_OUT_PWM_PRESCALER (800-1)

/* Auto-reload value used for PWM channels */
#define BSP_OUT_PWM_ARR (500-1)

/* TODO Change duty 0-100% to CCR register value */
#define BSP_OUT_DutyToCompare(X) ((X * BSP_OUT_PWM_ARR)/100)

/* Value used when starting PWM channel */
#define BSP_OUT_PWM_START_DUTY 0
typedef struct _T_BSP_OUT_CFG
{
    const T_IO     io;
    TIM_TypeDef*   tim;
    const uint8_t  ch;
    const uint32_t chmask;
    const uint32_t alt;
    const uint32_t clock;
    const volatile uint16_t* currentRawData;
    const uint32_t dkilis;
    const uint32_t sensRValue;
    const uint32_t faultLevel;  // Defined absolutly to remove non-needed calculations
}T_BSP_OUT_CFG;

static const T_BSP_OUT_CFG bspOutsCfg[OUT_ID_MAX] = 
{
    [OUT_ID_1] = 
    {
        .io = {PWM_SIG1_GPIO_Port, PWM_SIG1_Pin},
        .tim = TIM3,
        .ch = 4,
        .chmask = LL_TIM_CHANNEL_CH4,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM3,
        // TODO This should be later changed 
        .currentRawData = &(adc1RawData[0]),
        .dkilis = 38000,
        .sensRValue = 7020,
        .faultLevel = 12000
    },
    [OUT_ID_2] = 
    {
        .io = {PWM_SIG2_GPIO_Port, PWM_SIG2_Pin},
        .tim = TIM3,
        .ch = 3,
        .chmask = LL_TIM_CHANNEL_CH3,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM3,
        .currentRawData = &(adc1RawData[1]),
        .dkilis = 38000,
        .sensRValue = 6990,
        .faultLevel = 12000
    },
    [OUT_ID_3] = 
    {
        .io = {PWM_SIG3_GPIO_Port, PWM_SIG3_Pin},
        .tim = TIM3,
        .ch = 2,
        .chmask = LL_TIM_CHANNEL_CH2,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM3,
        .currentRawData = &(adc1RawData[2]),
        .dkilis = 38000,
        .sensRValue = 7025,
        .faultLevel = 12000
    },
    [OUT_ID_4] = 
    {
        .io = {PWM_SIG4_GPIO_Port, PWM_SIG4_Pin},
        .tim = TIM3,
        .ch = 1,
        .chmask = LL_TIM_CHANNEL_CH1,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM3,
        .currentRawData = &(adc1RawData[3]),
        .dkilis = 38000,
        .sensRValue = 2999,
        .faultLevel = 12000
    },
    [OUT_ID_5] = 
    {
        .io = {PWM_SIG5_GPIO_Port, PWM_SIG5_Pin},
        .tim = TIM4,
        .ch = 4,
        .chmask = LL_TIM_CHANNEL_CH4,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM4,
        .currentRawData = &(adc1RawData[4]),
        .dkilis = 50000,
        .sensRValue = 6940,
        .faultLevel = 12000
    },
    [OUT_ID_6] = 
    {
        .io = {PWM_SIG6_GPIO_Port, PWM_SIG6_Pin},
        .tim = TIM4,
        .ch = 3,
        .chmask = LL_TIM_CHANNEL_CH3,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM4,
        .currentRawData = &(adc1RawData[5]),
        .dkilis = 50000,
        .sensRValue = 6975,
        .faultLevel = 12000
    },
    [OUT_ID_7] = 
    {
        .io = {PWM_SIG7_GPIO_Port, PWM_SIG7_Pin},
        .tim = TIM4,
        .ch = 2,
        .chmask = LL_TIM_CHANNEL_CH2,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM4,
        .currentRawData = &(adc1RawData[6]),
        .dkilis = 50000,
        .sensRValue = 2334,
        .faultLevel = 12000
    },
    [OUT_ID_8] = 
    {
        .io = {PWM_SIG8_GPIO_Port, PWM_SIG8_Pin},
        .tim = TIM4,
        .ch = 1,
        .chmask = LL_TIM_CHANNEL_CH1,
        .alt = LL_GPIO_AF_2,
        .clock = LL_APB1_GRP1_PERIPH_TIM4,
        .currentRawData = &(adc1RawData[7]),
        .dkilis = 50000,
        .sensRValue = 6952,
        .faultLevel = 12000
    },
    [OUT_ID_9] = 
    {
        .currentRawData = &(VMUX_LP1Voltage[0]),
        .dkilis = 1830,
        .sensRValue = 2400
    },
    [OUT_ID_10] = 
    {
        .currentRawData = &(VMUX_LP1Voltage[1]),
        .dkilis = 830,
        .sensRValue = 2400
    },
    [OUT_ID_11] = 
    {
        .currentRawData = &(VMUX_LP1Voltage[2]),
        .dkilis = 830,
        .sensRValue = 2400
    },
    [OUT_ID_12] = 
    {
        .currentRawData = &(VMUX_LP1Voltage[3]),
        .dkilis = 1830,
        .sensRValue = 2400
    },
    [OUT_ID_13] = 
    {
        .currentRawData = &(VMUX_LP2Voltage[0]),
        .dkilis = 1830,
        .sensRValue = 2400
    },
    [OUT_ID_14] = 
    {
        .currentRawData = &(VMUX_LP2Voltage[1]),
        .dkilis = 830,
        .sensRValue = 2400
    },
    [OUT_ID_15] = 
    {
        .currentRawData = &(VMUX_LP2Voltage[2]),
        .dkilis = 830,
        .sensRValue = 2400
    },
    [OUT_ID_16] = 
    {
        .currentRawData = &(VMUX_LP2Voltage[3]),
        .dkilis = 1830,
        .sensRValue = 2400
    }
};

/// @brief Get I/O descriptor
/// @param id Output channel id [1..16] T_OUT_ID
/// @return T_IO descriptor
static inline T_IO BSP_OUT_GetIO(T_OUT_ID id)
{
    return bspOutsCfg[id].io;
}

uint32_t BSP_OUT_GetDkilis(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);
    return bspOutsCfg[id].dkilis;
}

uint32_t BSP_OUT_GetFaultLevel(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);
    return bspOutsCfg[id].faultLevel;
}

uint32_t BSP_OUT_CalcCurrent(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);
    uint32_t isVoltage = ((float)*(bspOutsCfg[id].currentRawData)/(float)4096)*(float)VDD_VALUE; 
    return isVoltage *  (bspOutsCfg[id].dkilis)/(bspOutsCfg[id].sensRValue);
}


static void BSP_OUT_SetTimerCompare(TIM_TypeDef* tim, uint8_t ch, uint32_t cmp)
{
    ASSERT(ch <= 4);
    switch (ch)
    {
    case 1:
        LL_TIM_OC_SetCompareCH1(tim, cmp);
        break;
    case 2:
        LL_TIM_OC_SetCompareCH2(tim, cmp);
        break;
    case 3:
        LL_TIM_OC_SetCompareCH3(tim, cmp);
        break;
    case 4:
        LL_TIM_OC_SetCompareCH4(tim, cmp);
        break;
    
    default:
        break;
    }
}

void BSP_OUT_InitPWM(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* IO config */
    GPIO_InitStruct.Pin = bspOutsCfg[id].io.pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    
    // Can be probably optimized out because all values AF2
    GPIO_InitStruct.Alternate = bspOutsCfg[id].alt; 
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;

    LL_GPIO_Init(bspOutsCfg[id].io.port, &GPIO_InitStruct);

    /* Timer config */
    if(LL_APB1_GRP1_IsEnabledClock(bspOutsCfg[id].clock) == 0)
    {
        LL_APB1_GRP1_EnableClock(bspOutsCfg[id].clock);

        LL_TIM_SetClockSource(bspOutsCfg[id].tim, LL_TIM_CLOCKSOURCE_INTERNAL);
        LL_TIM_SetCounterMode(bspOutsCfg[id].tim, LL_TIM_COUNTERMODE_UP);

        // TODO Change prescaler and arr 
        LL_TIM_SetPrescaler(bspOutsCfg[id].tim, BSP_OUT_PWM_PRESCALER);
        LL_TIM_SetAutoReload(bspOutsCfg[id].tim, BSP_OUT_PWM_ARR);

        /* Generate update event to change prescaler and arr immediately */
        LL_TIM_GenerateEvent_UPDATE(bspOutsCfg[id].tim);
        LL_TIM_ClearFlag_UPDATE(bspOutsCfg[id].tim);
    }

    /* Channel config */
    BSP_OUT_SetTimerCompare(bspOutsCfg[id].tim, bspOutsCfg[id].ch, BSP_OUT_PWM_START_DUTY);

    LL_TIM_OC_SetMode(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCPOLARITY_HIGH);

    LL_TIM_CC_EnableChannel(bspOutsCfg[id].tim, bspOutsCfg[id].chmask);

    if(LL_TIM_IsEnabledCounter(bspOutsCfg[id].tim) == 0)
    {
        LL_TIM_EnableCounter(bspOutsCfg[id].tim);
    }
}

void BSP_OUT_DeInitPWM(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);

    BSP_OUT_SetTimerCompare(bspOutsCfg[id].tim, bspOutsCfg[id].ch, BSP_OUT_PWM_START_DUTY);
    LL_TIM_CC_DisableChannel(bspOutsCfg[id].tim, bspOutsCfg[id].chmask);
    LL_TIM_OC_SetMode(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCMODE_FORCED_INACTIVE);
}

 void BSP_OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty)
{
    ASSERT( id < OUT_ID_MAX );
    BSP_OUT_SetTimerCompare(bspOutsCfg[id].tim, bspOutsCfg[id].ch, BSP_OUT_DutyToCompare(duty));
}

void BSP_OUT_SetStdState(T_OUT_ID id, bool state)
{
    ASSERT( id < OUT_ID_MAX);
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
    ASSERT( id < OUT_ID_MAX);
    ASSERT( batchId < OUT_ID_MAX);
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
    ASSERT( id < OUT_ID_MAX);
    ASSERT( batchId < OUT_ID_MAX);
    
    return ((BSP_OUT_GetIO(id).port) == (BSP_OUT_GetIO(batchId).port));
}

void BSP_OUT_SetMode(T_OUT_ID id, T_OUT_MODE mode)
{
    ASSERT( id < OUT_ID_MAX);
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
            BSP_OUT_InitPWM(id);
        break;

        default:
        break;
    }
}

uint32_t BSP_OUT_GetCurrentAdcValue(T_OUT_ID id)
{
    ASSERT( id < OUT_ID_MAX);

    if( bspOutsCfg[id].currentRawData != NULL)
    {
        return *(bspOutsCfg[id].currentRawData);
    }
    else
    {
        return 0xFFFFFFFF;
    }  
}

// TODO Unused
void BSP_OUT_Init(T_IO io)
{
    // Move initial GPIO init here
    // Pull down low
}

// TODO [MAJOR REWORK] Check PWM capabilites, change PWM frequency 