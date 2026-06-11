#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_ll_tim.h"
#include "logger.h"
#include "vmux.h"
#include "bsp_out.h"
#include "adc.h"
#include "adc_handler.h"

//#define LL_TIM_OC_SetCompare(TIM, CNUM, CMP) LL_TIM_OC_SetCompareCH##CNUM(TIM, CMP)
#define BSP_OUT_FAULT_ADC_LEVEL 4000

/* Current MCU main clock @80Mhz */
#define BSP_OUT_MAIN_CLK 80000000

/* PWM output frequency */
#define BSP_OUT_PWM_TARGET_FREQ 150

/* Prescaler value used for PWM channels */
#define BSP_OUT_PWM_PRESCALER (800-1)

/* Auto-reload value used for PWM channels */
#define BSP_OUT_PWM_ARR (BSP_OUT_MAIN_CLK / (BSP_OUT_PWM_PRESCALER * (2 * BSP_OUT_PWM_TARGET_FREQ)))

/* Change duty 0-100% to CCR register value */
#define BSP_OUT_DutyToCompare(X) ((X * BSP_OUT_PWM_ARR)/100)

/* Value used when starting PWM channel */
#define BSP_OUT_PWM_START_DUTY 0
typedef struct _T_BSP_OUT_CFG
{
    const T_IO     io;
    TIM_TypeDef*   tim;     // PWM timer
    const uint8_t  ch;      // PWM timer channel
    const uint32_t chmask;  // PWM timer channel mask
    const uint32_t alt;     // GPIO alternate function for PWM
    const uint32_t clock;   // PWM timer clock
    const IRQn_Type      timIRQn; // PWM timer IRQ number
    const volatile uint16_t* currentRawData;
    const uint32_t dkilis;
    const uint32_t sensRValue;
    const uint32_t faultLevel;  // Defined absolutly to remove non-needed calculations
}T_BSP_OUT_CFG;

typedef struct _T_BSP_OUT_REG
{
    bool isPWM; // Is output configured as PWM?
    uint8_t duty; // BSP PWM duty %
}
T_BSP_OUT_REG;

#if BOARD_VER == PDMS_V4_2
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
        .currentRawData = &(adc1RawData[0]),
        .dkilis = 38000,
        .sensRValue = 8200,
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
#elif BOARD_VER == PDMS_V4_3
static const T_BSP_OUT_CFG bspOutsCfg[OUT_ID_MAX] = 
{
    [OUT_ID_1] = 
    {
        .io = {PWM_SIG1_GPIO_Port, PWM_SIG1_Pin},
        .tim = TIM8,
        .ch = 4,
        .chmask = LL_TIM_CHANNEL_CH4,
        .alt = LL_GPIO_AF_3,
        .clock = LL_APB2_GRP1_PERIPH_TIM8,
        .timIRQn = TIM8_IRQn,
        .currentRawData = &(adc1AvgData[0]),
        .dkilis = 38000,
        .sensRValue = 3900,
        .faultLevel = 12000
    },
    [OUT_ID_2] = 
    {
        .io = {PWM_SIG2_GPIO_Port, PWM_SIG2_Pin},
        .tim = TIM8,
        .ch = 3,
        .chmask = LL_TIM_CHANNEL_CH3,
        .alt = LL_GPIO_AF_3,
        .clock = LL_APB2_GRP1_PERIPH_TIM8,
        .timIRQn = TIM8_IRQn,
        .currentRawData = &(adc1AvgData[1]),
        .dkilis = 38000,
        .sensRValue = 3900,
        .faultLevel = 12000
    },
    [OUT_ID_3] = 
    {
        .io = {PWM_SIG3_GPIO_Port, PWM_SIG3_Pin},
        .tim = TIM8,
        .ch = 2,
        .chmask = LL_TIM_CHANNEL_CH2,
        .alt = LL_GPIO_AF_3,
        .clock = LL_APB2_GRP1_PERIPH_TIM8,
        .timIRQn = TIM8_IRQn,
        .currentRawData = &(adc1AvgData[2]),
        .dkilis = 38000,
        .sensRValue = 3900,
        .faultLevel = 12000
    },
    [OUT_ID_4] = 
    {
        .io = {PWM_SIG4_GPIO_Port, PWM_SIG4_Pin},
        .tim = TIM8,
        .ch = 1,
        .chmask = LL_TIM_CHANNEL_CH1,
        .alt = LL_GPIO_AF_3,
        .clock = LL_APB2_GRP1_PERIPH_TIM8,
        .timIRQn = TIM8_IRQn,
        .currentRawData = &(adc1AvgData[3]),
        .dkilis = 38000,
        .sensRValue = 3900,
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
        .timIRQn = TIM4_IRQn,
        .currentRawData = &(adc1AvgData[4]),
        .dkilis = 38000,
        .sensRValue = 3900,
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
        .timIRQn = TIM4_IRQn,
        .currentRawData = &(adc1AvgData[5]),
        .dkilis = 38000,
        .sensRValue = 3900,
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
        .timIRQn = TIM4_IRQn,
        .currentRawData = &(adc1AvgData[6]),
        .dkilis = 38000,
        .sensRValue = 3900,
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
        .timIRQn = TIM4_IRQn,
        .currentRawData = &(adc1AvgData[7]),
        .dkilis = 38000,
        .sensRValue = 3900,
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

volatile T_BSP_OUT_REG bspOutsReg[OUT_ID_MAX];

#endif

/// @brief Get I/O descriptor
/// @param id Output channel id [1..16] T_OUT_ID
/// @return T_IO descriptor
static inline T_IO BSP_OUT_GetIO(T_OUT_ID id)
{
    return bspOutsCfg[id].io;
}

uint32_t BSP_OUT_GetDkilis(T_OUT_ID id)
{
    return bspOutsCfg[id].dkilis;
}

uint32_t BSP_OUT_GetFaultLevel(T_OUT_ID id)
{
    return bspOutsCfg[id].faultLevel;
}

uint32_t BSP_OUT_CalcCurrent(T_OUT_ID id)
{
    uint32_t isVoltage = ((float)*(bspOutsCfg[id].currentRawData)/(float)4096)*(float)VDD_VALUE; 
    return isVoltage *  (bspOutsCfg[id].dkilis)/(bspOutsCfg[id].sensRValue) * (bspOutsReg[id].isPWM ? (bspOutsReg[id].duty/100.0) : 1);
}

bool BSP_OUT_IsCurrentFault(T_OUT_ID id)
{
    return (*(bspOutsCfg[id].currentRawData) >= BSP_OUT_FAULT_ADC_LEVEL);
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

void BSP_OUT_InitTimers(void)
{   
    // TIM8
    if(LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_TIM8) == 0)
    {
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);

        LL_TIM_SetClockSource(TIM8, LL_TIM_CLOCKSOURCE_INTERNAL);
        LL_TIM_SetCounterMode(TIM8, LL_TIM_COUNTERMODE_CENTER_UP_DOWN);

        LL_TIM_EnableARRPreload(TIM8);
        // Current solution aims 175 Hz
        // Modify BSP_OUT_PWM_TARGET_FREQ if needed
        LL_TIM_SetPrescaler(TIM8, BSP_OUT_PWM_PRESCALER);
        LL_TIM_SetAutoReload(TIM8, BSP_OUT_PWM_ARR);

        //LL_TIM_SetRepetitionCounter(TIM8, 1);

        /* Generate update event to change prescaler and arr immediately */
        LL_TIM_GenerateEvent_UPDATE(TIM8);
        LL_TIM_ClearFlag_UPDATE(TIM8);

        // HAL_NVIC_SetPriority(TIM8_IRQn, 3, 0);
        // HAL_NVIC_EnableIRQ(TIM8_IRQn);
    }

    /* ADC Injected channel trigger */
    LL_TIM_OC_SetCompareCH6(TIM8, 1);

    LL_TIM_OC_SetMode(TIM8, LL_TIM_CHANNEL_CH6, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM8, LL_TIM_CHANNEL_CH6, LL_TIM_OCPOLARITY_HIGH);

    LL_TIM_CC_EnableChannel(TIM8, LL_TIM_CHANNEL_CH6);

    LL_TIM_EnableIT_UPDATE(TIM8);
    // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_HIGH);

    // TIM4
    if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_TIM4) == 0)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

        LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);
        LL_TIM_SetCounterMode(TIM4, LL_TIM_COUNTERMODE_CENTER_UP_DOWN);

        LL_TIM_EnableARRPreload(TIM4);
        // Current solution aims 175 Hz
        // Modify BSP_OUT_PWM_TARGET_FREQ if needed
        LL_TIM_SetPrescaler(TIM4, BSP_OUT_PWM_PRESCALER);
        LL_TIM_SetAutoReload(TIM4, BSP_OUT_PWM_ARR);

        /* Generate update event to change prescaler and arr immediately */
        LL_TIM_GenerateEvent_UPDATE(TIM4);
        LL_TIM_ClearFlag_UPDATE(TIM4);

        // HAL_NVIC_SetPriority(bspOutsCfg[id].timIRQn, 3, 0);
        // HAL_NVIC_EnableIRQ(bspOutsCfg[id].timIRQn);
    }

    LL_TIM_SetTriggerOutput(TIM8, LL_TIM_TRGO_UPDATE);
    LL_TIM_SetTriggerOutput2(TIM8, LL_TIM_TRGO2_OC6);
    // TIM4 is slave now
    LL_TIM_SetTriggerInput(TIM4, LL_TIM_TS_ITR3);
    LL_TIM_SetSlaveMode(TIM4, LL_TIM_SLAVEMODE_RESET);

    // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);

    // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
    // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_HIGH);

    LL_TIM_SetCounter(TIM4, 0);
    LL_TIM_SetCounter(TIM8, 0);
    
    LL_TIM_ClearFlag_UPDATE(TIM8);
    LL_TIM_ClearFlag_UPDATE(TIM4);

    // Slave start -- listen to events from master
    LL_TIM_EnableCounter(TIM4);

    // Master start
    LL_TIM_EnableCounter(TIM8);

    // This is required to unlock TIM8 outputs, they are locked by MOE by default
    LL_TIM_EnableAllOutputs(TIM8);
}

// void BSP_OUT_InitTimers(void)
// {   
//     // TIM3
//     if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_TIM3) == 0)
//     {
//         LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

//         LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
//         LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_CENTER_UP_DOWN);

//         // Current solution aims 175 Hz
//         // Modify BSP_OUT_PWM_TARGET_FREQ if needed
//         LL_TIM_SetPrescaler(TIM3, BSP_OUT_PWM_PRESCALER);
//         LL_TIM_SetAutoReload(TIM3, BSP_OUT_PWM_ARR);

//         LL_TIM_SetRepetitionCounter(TIM3, 1);

//         /* Generate update event to change prescaler and arr immediately */
//         LL_TIM_GenerateEvent_UPDATE(TIM3);
//         LL_TIM_ClearFlag_UPDATE(TIM3);

//         // HAL_NVIC_SetPriority(bspOutsCfg[id].timIRQn, 3, 0);
//         // HAL_NVIC_EnableIRQ(bspOutsCfg[id].timIRQn);
//     }

//     // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_HIGH);

//     // TIM4
//     if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_TIM4) == 0)
//     {
//         LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

//         LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);
//         LL_TIM_SetCounterMode(TIM4, LL_TIM_COUNTERMODE_CENTER_UP_DOWN);

//         // Current solution aims 175 Hz
//         // Modify BSP_OUT_PWM_TARGET_FREQ if needed
//         LL_TIM_SetPrescaler(TIM4, BSP_OUT_PWM_PRESCALER);
//         LL_TIM_SetAutoReload(TIM4, BSP_OUT_PWM_ARR);

//         /* Generate update event to change prescaler and arr immediately */
//         LL_TIM_GenerateEvent_UPDATE(TIM4);
//         LL_TIM_ClearFlag_UPDATE(TIM4);

//         // HAL_NVIC_SetPriority(bspOutsCfg[id].timIRQn, 3, 0);
//         // HAL_NVIC_EnableIRQ(bspOutsCfg[id].timIRQn);
//     }

//     LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_UPDATE);
//     // TIM4 is slave now
//     LL_TIM_SetTriggerInput(TIM4, LL_TIM_TS_ITR2);
//     LL_TIM_SetSlaveMode(TIM4, LL_TIM_SLAVEMODE_RESET);

//     // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);

//     // LL_TIM_OC_SetMode(TIM4, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
//     // LL_TIM_OC_SetPolarity(TIM4, LL_TIM_CHANNEL_CH4, LL_TIM_OCPOLARITY_HIGH);

//     LL_TIM_SetCounter(TIM4, 0);
//     LL_TIM_SetCounter(TIM3, 0);
    
//     LL_TIM_ClearFlag_UPDATE(TIM3);
//     LL_TIM_ClearFlag_UPDATE(TIM4);

//     // Slave start -- listen to events from master
//     LL_TIM_EnableCounter(TIM4);

//     // Master start
//     LL_TIM_SetCounter(TIM3, 0);
//     LL_TIM_EnableCounter(TIM3);

// }

// void BSP_OUT_InitPWM(T_OUT_ID id)
// {
//     ASSERT( id < OUT_ID_MAX);
//     LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

//     /* IO config */
//     GPIO_InitStruct.Pin = bspOutsCfg[id].io.pin;
//     GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
//     GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    
//     // Can be probably optimized out because all values AF2
//     GPIO_InitStruct.Alternate = bspOutsCfg[id].alt; 
//     GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;

//     LL_GPIO_Init(bspOutsCfg[id].io.port, &GPIO_InitStruct);

//     /* Timer config */
//     if(LL_APB1_GRP1_IsEnabledClock(bspOutsCfg[id].clock) == 0)
//     {
//         LL_APB1_GRP1_EnableClock(bspOutsCfg[id].clock);

//         LL_TIM_SetClockSource(bspOutsCfg[id].tim, LL_TIM_CLOCKSOURCE_INTERNAL);
//         LL_TIM_SetCounterMode(bspOutsCfg[id].tim, LL_TIM_COUNTERMODE_UP);

//         // Current solution aims 175 Hz
//         // Modify BSP_OUT_PWM_TARGET_FREQ if needed
//         LL_TIM_SetPrescaler(bspOutsCfg[id].tim, BSP_OUT_PWM_PRESCALER);
//         LL_TIM_SetAutoReload(bspOutsCfg[id].tim, BSP_OUT_PWM_ARR);

//         /* Generate update event to change prescaler and arr immediately */
//         LL_TIM_GenerateEvent_UPDATE(bspOutsCfg[id].tim);
//         LL_TIM_ClearFlag_UPDATE(bspOutsCfg[id].tim);

//         HAL_NVIC_SetPriority(bspOutsCfg[id].timIRQn, 3, 0);
//         HAL_NVIC_EnableIRQ(bspOutsCfg[id].timIRQn);
//     }

//     /* Channel config */
//     BSP_OUT_SetTimerCompare(bspOutsCfg[id].tim, bspOutsCfg[id].ch, BSP_OUT_PWM_START_DUTY);

//     LL_TIM_OC_SetMode(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCMODE_PWM1);
//     LL_TIM_OC_SetPolarity(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCPOLARITY_HIGH);

//     // Enable update interrupt to detect rising edge
//     LL_TIM_EnableIT_UPDATE(bspOutsCfg[id].tim);

//     // Enable capture-compare interrupt based on channel
//     if (bspOutsCfg[id].chmask == LL_TIM_CHANNEL_CH1)      LL_TIM_EnableIT_CC1(bspOutsCfg[id].tim);
//     else if (bspOutsCfg[id].chmask == LL_TIM_CHANNEL_CH2) LL_TIM_EnableIT_CC2(bspOutsCfg[id].tim);
//     else if (bspOutsCfg[id].chmask == LL_TIM_CHANNEL_CH3) LL_TIM_EnableIT_CC3(bspOutsCfg[id].tim);
//     else if (bspOutsCfg[id].chmask == LL_TIM_CHANNEL_CH4) LL_TIM_EnableIT_CC4(bspOutsCfg[id].tim);

//     LL_TIM_CC_EnableChannel(bspOutsCfg[id].tim, bspOutsCfg[id].chmask);

//     if(LL_TIM_IsEnabledCounter(bspOutsCfg[id].tim) == 0)
//     {
//         LL_TIM_EnableCounter(bspOutsCfg[id].tim);
//     }
// }

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

    /* Channel config */
    BSP_OUT_SetTimerCompare(bspOutsCfg[id].tim, bspOutsCfg[id].ch, BSP_OUT_PWM_START_DUTY);

    LL_TIM_OC_SetMode(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(bspOutsCfg[id].tim, bspOutsCfg[id].chmask, LL_TIM_OCPOLARITY_HIGH);


    LL_TIM_CC_EnableChannel(bspOutsCfg[id].tim, bspOutsCfg[id].chmask);
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
    bspOutsReg[id].duty = duty;
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
            bspOutsReg[id].isPWM = FALSE;
        break;
        
        case OUT_MODE_STD:
            LL_GPIO_ResetOutputPin(io.port, io.pin);
            GPIO_InitStruct.Pin = io.pin;
            GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
            GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            LL_GPIO_Init(io.port, &GPIO_InitStruct);
            bspOutsReg[id].isPWM = FALSE;
        break;

        case OUT_MODE_PWM:
            LL_GPIO_ResetOutputPin(io.port, io.pin);
            BSP_OUT_InitPWM(id);
            bspOutsReg[id].isPWM = TRUE;
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

void BSP_OUT_Init(T_IO io)
{
    // Move initial GPIO init here
    // Pull down low
}


// __attribute__((always_inline)) -- Works only with compiler optimizations enabled
__attribute__((always_inline)) bool BSP_OUT_IsPWM(T_OUT_ID id)
{
    return bspOutsReg[id].isPWM;
}

// void TIM3_IRQHandler(void)
// {
//     // Detect PWM rising edge
//     if (LL_TIM_IsActiveFlag_UPDATE(TIM3) == 1) 
//     {
//         LL_TIM_ClearFlag_UPDATE(TIM3);
//         bspOutsReg[OUT_ID_1].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_2].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_3].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_4].senseValidPWM = TRUE;
//     }

//     // Detect PWM falling edge
//     if (LL_TIM_IsActiveFlag_CC1(TIM3) == 1) 
//     {
//         LL_TIM_ClearFlag_CC1(TIM3);
//         bspOutsReg[OUT_ID_4].senseValidPWM = FALSE;
        
//     }
//     else if (LL_TIM_IsActiveFlag_CC2(TIM3) == 1) 
//     {
//         LL_TIM_ClearFlag_CC2(TIM3);
//         bspOutsReg[OUT_ID_3].senseValidPWM = FALSE;
        
//     }
//     else if (LL_TIM_IsActiveFlag_CC3(TIM3) == 1) 
//     {
//         LL_TIM_ClearFlag_CC3(TIM3);
//         bspOutsReg[OUT_ID_2].senseValidPWM = FALSE;
        
//     }
//     else if (LL_TIM_IsActiveFlag_CC4(TIM3) == 1) 
//     {
//         LL_TIM_ClearFlag_CC4(TIM3);
//         bspOutsReg[OUT_ID_1].senseValidPWM = FALSE;
        
//     }
// }

// void TIM4_IRQHandler(void)
// {
//     // Detect PWM rising edge
//     if (LL_TIM_IsActiveFlag_UPDATE(TIM4) == 1) 
//     {
//         LL_TIM_ClearFlag_UPDATE(TIM4);
//         bspOutsReg[OUT_ID_5].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_6].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_7].senseValidPWM = TRUE;
//         bspOutsReg[OUT_ID_8].senseValidPWM = TRUE;
        
//     }

//     // Detect PWM falling edge
//     if (LL_TIM_IsActiveFlag_CC1(TIM4) == 1) 
//     {
//         LL_TIM_ClearFlag_CC1(TIM4);
//         bspOutsReg[OUT_ID_8].senseValidPWM = FALSE;
        
//     }
//     else if (LL_TIM_IsActiveFlag_CC2(TIM4) == 1) 
//     {
//         LL_TIM_ClearFlag_CC2(TIM4);
//         bspOutsReg[OUT_ID_7].senseValidPWM = FALSE;
//     }
//     else if (LL_TIM_IsActiveFlag_CC3(TIM4) == 1) 
//     {
//         LL_TIM_ClearFlag_CC3(TIM4);
//         bspOutsReg[OUT_ID_6].senseValidPWM = FALSE;
        
//     }
//     else if (LL_TIM_IsActiveFlag_CC4(TIM4) == 1) 
//     {
//         LL_TIM_ClearFlag_CC4(TIM4);
//         bspOutsReg[OUT_ID_5].senseValidPWM = FALSE;

//     }
// }



// void TIM8_IRQHandler(void)
// {
//     LL_TIM_ClearFlag_UPDATE(TIM8);
// }