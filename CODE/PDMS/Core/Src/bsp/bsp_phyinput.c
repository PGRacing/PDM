#include "bsp_phyinput.h"
#include "main.h"
#include "adc.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "FreeRTOS.h"

/// USER DEFINES
#define PHY_INPUT_VOLTAGE_DIVIDER 1.836f // Voltage divider on board scales max input voltage from 0-5V to 0-3.3V range for ADC
#define PHY_INPUT_EMA_ALPHA 0.3f 
/// MACRO FUNCTIONS

/// FUNCTION PROTOTYPES

/// PRIVATE TYPEDEFS
typedef struct _T_BSP_PHYIN_CFG
{
    const T_IO        io;
    uint16_t * const  rawData; // pointer to rawData from input
}T_BSP_PHYIN_CFG;

/// @brief Physical inputs configuration
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

/// @brief Physical inputs register
T_BSP_IN_REG bspPhyInputsReg[IN_PHY_MAX] =
    {
        [IN_PHY_LOC_1] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_2] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_3] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_4] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_5] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_6] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_7] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
        [IN_PHY_LOC_8] =
            {
                .rawValue = 0,
                .schmittState = FALSE,
                .voltageValue = 0,
                .emaVoltageValue = 0,
            },
};

/// @brief Evaluate values of all physical inputs
/// @param  None
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
        // Calculate EMA filtered voltage value
        bspPhyInputsReg[i].emaVoltageValue = (uint_fast16_t)(PHY_INPUT_EMA_ALPHA * bspPhyInputsReg[i].voltageValue + (1.0f - PHY_INPUT_EMA_ALPHA) * bspPhyInputsReg[i].emaVoltageValue);
    } 
    vPortExitCritical();
}

/// @brief Get value of physical input as schmitt trigger
/// @param location location of the physical input
/// @return schmitt state of the physical input
bool BSP_PHYIN_GetValueSchmitt(T_IN_PHY_LOC location)
{
    ASSERT(location < IN_PHY_MAX);
    return bspPhyInputsReg[location].schmittState;
}

/// @brief Get value of physical input as analog voltage
/// @param location location of the physical input
/// @return The analog voltage value in mV (0-5000 mV)
uint_fast16_t BSP_PHYIN_GetValueAnalog(T_IN_PHY_LOC location)
{
    ASSERT(location < IN_PHY_MAX);
    return bspPhyInputsReg[location].emaVoltageValue;
}