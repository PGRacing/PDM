#include "bsp_caninput.h"
#include "bsp_cmninput.h"
#include "main.h"
#include "logger.h"
#include <string.h>

/// USER DEFINES

/// MACRO FUNCTIONS

/// FUNCTION PROTOTYPES
static void BSP_CANIN_EvaluateValue(T_IN_CAN_LOC loc);

/// PRIVATE TYPEDEFS

static const uint8_t canInputSize[] =
{
    [CAN_INPUT_TYPE_BOOL] = 1,
    [CAN_INPUT_TYPE_UINT16] = 2,
    [CAN_INPUT_TYPE_UINT32] = 4,
    [CAN_INPUT_TYPE_INT16] = 2,
    [CAN_INPUT_TYPE_INT32] = 4,
    [CAN_INPUT_TYPE_FLOAT] = 2,
};

/// @brief CAN inputs configuration
T_BSP_CANIN_CFG bspCanInputsCfg[IN_CAN_MAX] =
{
    [IN_CAN_LOC_1] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x730,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_UINT16,
        },
    [IN_CAN_LOC_2] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x901,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_INT32,
        },
    [IN_CAN_LOC_3] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x902,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_BOOL,
        },
    [IN_CAN_LOC_4] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_5] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_6] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_7] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_8] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_9] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_10] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_11] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_12] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_13] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_14] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_15] =
        {
            .isUsed = FALSE,
        },
    [IN_CAN_LOC_16] =
        {
            .isUsed = FALSE,
        }
};


/// @brief CAN inputs register
T_BSP_IN_REG bspCanInputsReg[IN_CAN_MAX] =
{
    [IN_CAN_LOC_1] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_2] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_3] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_4] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_5] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_6] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_7] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_8] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_9] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_10] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_11] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_12] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_13] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_14] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_15] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        },
    [IN_CAN_LOC_16] =
        {
            .rawValue = 0,
            .schmittState = FALSE,
            .voltageValue = 0,
        }
};

/// @brief Dispatch a received CAN frame to the appropriate input based on its ID and instance
/// @param canInstance The CAN instance to which the frame was received
/// @param id The ID of the received CAN frame
/// @param raw A pointer to the raw data of the received CAN frame
/// @param dlc The data length code of the received CAN frame
void BSP_CANIN_DispatchFrame(T_CANH_INSTANCE canInstance, uint32_t id, uint8_t* raw, uint8_t dlc)
{
    for(uint8_t i = 0; i < IN_CAN_MAX; i++)
    {
        if(bspCanInputsCfg[i].isUsed == FALSE)
        {
            continue;
        }

        if(bspCanInputsCfg[i].canInstance == canInstance && bspCanInputsCfg[i].canId == id)
        {
            if(canInputSize[bspCanInputsCfg[i].dataType] + bspCanInputsCfg[i].offset > dlc)
            {
                LOG_WARN("BSP_CANIN:: Received frame with insufficient data length for input");
                continue;
            }
            // Copy the data from input frame
            memcpy(&(bspCanInputsReg[i].rawValue), raw + bspCanInputsCfg[i].offset, canInputSize[bspCanInputsCfg[i].dataType]);
            BSP_CANIN_EvaluateValue(i);
        }
    }
}

/// @brief Evaluates the value of a CAN input based on its configuration
/// @param loc The location of the CAN input to evaluate
static void BSP_CANIN_EvaluateValue(T_IN_CAN_LOC loc)
{
    
    if(bspCanInputsCfg[loc].isUsed == FALSE)
    {
        return;
    }

    if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_BOOL)
    {
        const bool value = *((bool*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value ? 5000 : 0;   
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_UINT16)
    {
        const uint16_t value = *((uint16_t*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value * 5000 / 65535; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_INT16)
    {
        const int16_t value = *((int16_t*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value * 5000 / 32767; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_UINT32)
    {
        const uint32_t value = *((uint32_t*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value * 5000 / 4294967295; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_INT32)
    {
        const int32_t value = *((int32_t*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value * 5000 / 2147483647; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_FLOAT)
    {
        const float value = *((float*)(&bspCanInputsReg[loc].rawValue));
        bspCanInputsReg[loc].voltageValue = value; // Assume float is already in the 0-5000 mV range
    }
    else
    {
        LOG_WARN("BSP_CANIN:: Unsupported data type for input 0");
    }

    if(bspCanInputsReg[loc].voltageValue > IN_SCHMITT_HIGH_THRESHOLD)
    {
        bspCanInputsReg[loc].schmittState = TRUE;
    }
    else if(bspCanInputsReg[loc].voltageValue < IN_SCHMITT_LOW_THRESHOLD)
    {
        bspCanInputsReg[loc].schmittState = FALSE;
    }

}

/// @brief Get value of CAN input as schmitt trigger
/// @param location The location of the CAN input
/// @return The schmitt trigger value
bool BSP_CANIN_GetValueSchmitt(T_IN_CAN_LOC location)
{
    ASSERT(location < IN_CAN_MAX);
    return bspCanInputsReg[location].schmittState;
}

/// @brief Get value of CAN input as analog voltage
/// @param location The location of the CAN input
/// @return The analog voltage value in mV (0-5000 mV)
uint_fast16_t BSP_CANIN_GetValueAnalog(T_IN_CAN_LOC location)
{
    ASSERT(location < IN_CAN_MAX);
    return bspCanInputsReg[location].voltageValue;
}