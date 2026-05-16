#include "bsp_phyinput.h"
#include "main.h"
#include "adc.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "FreeRTOS.h"

#define PHY_INPUT_VOLTAGE_DIVIDER 1.53

T_BSP_PHYIN_CFG bspPhyInputsCfg[IN_PHY_MAX] =
    {
        [IN_PHY_LOC_1] =
            {
                .io = {IO_CONN1_GPIO_Port, IO_CONN1_Pin},
                .rawData = &(adc2RawData[0]),
            },
        [IN_PHY_LOC_2] =
            {
                .io = {IO_CONN2_GPIO_Port, IO_CONN2_Pin},
                .rawData = &(adc2RawData[1]),
            },
        [IN_PHY_LOC_3] =
            {
                .io = {IO_CONN3_GPIO_Port, IO_CONN3_Pin},
                .rawData = &(adc2RawData[2]),
            },
        [IN_PHY_LOC_4] =
            {
                .io = {IO_CONN4_GPIO_Port, IO_CONN4_Pin},
                .rawData = &(adc2RawData[3]),
            },
        [IN_PHY_LOC_5] =
            {
                .io = {IO_CONN5_GPIO_Port, IO_CONN5_Pin},
                .rawData = &(adc2RawData[4]),
            },
        [IN_PHY_LOC_6] =
            {
                .io = {IO_CONN6_GPIO_Port, IO_CONN6_Pin},
                .rawData = &(adc2RawData[5]),
            },
        [IN_PHY_LOC_7] =
            {
                .io = {IO_CONN7_GPIO_Port, IO_CONN7_Pin},
                .rawData = &(adc2RawData[6]),
            },
        [IN_PHY_LOC_8] =
            {
                .io = {IO_CONN8_GPIO_Port, IO_CONN8_Pin},
                .rawData = &(adc2RawData[7]),
            },
};

T_BSP_IN_REG bspPhyInputsReg[IN_PHY_MAX] =
    {
        [IN_PHY_LOC_1] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_2] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_3] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_4] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_5] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_6] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_7] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
        [IN_PHY_LOC_8] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
            },
};


void BSP_PHYIN_EvaluateValues(void)
{
    vPortEnterCritical();
    // TODO Add mutex for adc2RawData
    for(uint8_t i = 0; i < IN_PHY_MAX; i++)
    {
        if(bspPhyInputsCfg[i].rawData != NULL)
        {
            /* TODO Add debouncing for phy digital inputs */
            if(*(bspPhyInputsCfg[i].rawData) > IN_SCHMITT_HIGH_THRESHOLD)
            {
                bspPhyInputsReg[i].schmittState = TRUE;
            }
            else if(*(bspPhyInputsCfg[i].rawData) < IN_SCHMITT_LOW_THRESHOLD)
            {
                bspPhyInputsReg[i].schmittState = FALSE;
            }
        }

        // Assign analog value
        bspPhyInputsReg[i].rawValue = *(bspPhyInputsCfg[i].rawData);
        // Calculate voltage value
        bspPhyInputsReg[i].voltageValue = (((float)*(bspPhyInputsCfg[i].rawData)*PHY_INPUT_VOLTAGE_DIVIDER)/(float)4096)*(float)VDD_VALUE;
    } 
    vPortExitCritical();
}

bool BSP_PHYIN_GetValueSchmitt(T_IN_PHY_LOC location)
{
    ASSERT(location < IN_PHY_MAX);
    return bspPhyInputsReg[location].schmittState;
}

uint_fast16_t BSP_PHYIN_GetValueAnalog(T_IN_PHY_LOC location)
{
    ASSERT(location < IN_PHY_MAX);
    return bspPhyInputsReg[location].voltageValue;
}