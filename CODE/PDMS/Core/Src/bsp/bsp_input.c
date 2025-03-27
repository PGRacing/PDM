#include "bsp_input.h"
#include "main.h"
#include "adc.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "FreeRTOS.h"

#define IN_SCHMITT_HIGH_THRESHOLD 2500
#define IN_SCHMITT_LOW_THRESHOLD 1500

T_BSP_IN_CFG bspInputsCfg[IN_PHY_MAX] =
    {
        [0] =
            {
                .id = IN_PHY_ID_1,
                .io = {IO_CONN1_GPIO_Port, IO_CONN1_Pin},
                .rawData = &(adc2RawData[0]),
            },
        [1] =
            {
                .id = IN_PHY_ID_2,
                .io = {IO_CONN2_GPIO_Port, IO_CONN2_Pin},
                .rawData = &(adc2RawData[1]),
            },
        [2] =
            {
                .id = IN_PHY_ID_3,
                .io = {IO_CONN3_GPIO_Port, IO_CONN3_Pin},
                .rawData = &(adc2RawData[2]),
            },
        [3] =
            {
                .id = IN_PHY_ID_4,
                .io = {IO_CONN4_GPIO_Port, IO_CONN4_Pin},
                .rawData = &(adc2RawData[3]),
            },
        [4] =
            {
                .id = IN_PHY_ID_5,
                .io = {IO_CONN5_GPIO_Port, IO_CONN5_Pin},
                .rawData = &(adc2RawData[4]),
            },
        [5] =
            {
                .id = IN_PHY_ID_6,
                .io = {IO_CONN6_GPIO_Port, IO_CONN6_Pin},
                .rawData = &(adc2RawData[5]),
            },
        [6] =
            {
                .id = IN_PHY_ID_7,
                .io = {IO_CONN7_GPIO_Port, IO_CONN7_Pin},
                .rawData = &(adc2RawData[6]),
            },
        [7] =
            {
                .id = IN_PHY_ID_8,
                .io = {IO_CONN8_GPIO_Port, IO_CONN8_Pin},
                .rawData = &(adc2RawData[7]),
            },
};

T_BSP_IN_REG bspInputsReg[IN_PHY_MAX] =
    {
        [0] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [1] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [2] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [3] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [4] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [5] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [6] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
        [7] =
            {
                .analogValue = 0,
                .schmittState = FALSE,
            },
};


void BSP_IN_EvaluateValues(void)
{
    vPortEnterCritical();
    // TODO Add mutex for adc2RawData
    for(uint8_t i = 0; i < IN_PHY_MAX; i++)
    {
        if(bspInputsCfg[i].rawData != NULL)
        {
            /* TODO Add debouncing for phy digital inputs */
            if(*(bspInputsCfg[i].rawData) > IN_SCHMITT_HIGH_THRESHOLD)
            {
                bspInputsReg[i].schmittState = TRUE;
            }
            else if(*(bspInputsCfg[i].rawData) < IN_SCHMITT_LOW_THRESHOLD)
            {
                bspInputsReg[i].schmittState = FALSE;
            }
        }

        // Assign analog value
        bspInputsReg[i].analogValue = *(bspInputsCfg[i].rawData);
    } 
    vPortExitCritical();
}

bool BSP_IN_GetValueSchmitt(T_INPUT_ID id)
{
    return bspInputsReg[id].schmittState;
}

uint_fast16_t BSP_IN_GetValueAnalog(T_INPUT_ID id)
{
    return bspInputsReg[id].analogValue;
}