#include "bsp_caninput.h"
#include "bsp_cmninput.h"
#include "main.h"
#include "logger.h"
#include <string.h>

static const uint8_t canInputSize[] =
{
    [CAN_INPUT_TYPE_BOOL] = 1,
    [CAN_INPUT_TYPE_UINT16] = 2,
    [CAN_INPUT_TYPE_UINT32] = 4,
    [CAN_INPUT_TYPE_INT16] = 2,
    [CAN_INPUT_TYPE_INT32] = 4,
    [CAN_INPUT_TYPE_FLOAT] = 2,
};

T_BSP_CANIN_CFG bspCanInputsCfg[] =
{
    [IN_CAN_LOC_1] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x730,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_UINT16,
            .rawData = 0,
        },
    [IN_CAN_LOC_2] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x901,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_INT32,
            .rawData = 0,
        },
    [IN_CAN_LOC_3] =
        {
            .isUsed = TRUE,
            .canInstance = CANH_INSTANCE_2,
            .canId = 0x902,
            .offset = 0,
            .dataType = CAN_INPUT_TYPE_BOOL,
            .rawData = 0,
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

T_BSP_IN_REG bspCanInputsReg[] =
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


static void BSP_CANIN_EvaluateValue(T_IN_CAN_LOC loc);

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
            memcpy(&(bspCanInputsCfg[i].rawData), raw + bspCanInputsCfg[i].offset, canInputSize[bspCanInputsCfg[i].dataType]);
            BSP_CANIN_EvaluateValue(i);
        }
    }
}

// Change to evaluate single value and later on update when some message enters into queue
// Later on interpreted as voltage so in range of ADC 0-5000mV
static void BSP_CANIN_EvaluateValue(T_IN_CAN_LOC loc)
{
    
    if(bspCanInputsCfg[loc].isUsed == FALSE)
    {
        return;
    }

    if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_BOOL)
    {
        bspCanInputsReg[loc].rawValue = bspCanInputsReg[loc].schmittState ? 1 : 0;
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].schmittState ? 5000 : 0;   
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_UINT16)
    {
        bspCanInputsReg[loc].rawValue = *((uint16_t*)(&bspCanInputsCfg[loc].rawData));
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].rawValue * 5000 / 65535; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_INT16)
    {
        bspCanInputsReg[loc].rawValue = *((int16_t*)(&bspCanInputsCfg[loc].rawData));
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].rawValue * 5000 / 32767; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_UINT32)
    {
        bspCanInputsReg[loc].rawValue = *((uint32_t*)(&bspCanInputsCfg[loc].rawData));
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].rawValue * 5000 / 4294967295; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_INT32)
    {
        bspCanInputsReg[loc].rawValue = *((int32_t*)(&bspCanInputsCfg[loc].rawData));
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].rawValue * 5000 / 2147483647; // Scale to 0-5000 mV
    }
    else if(bspCanInputsCfg[loc].dataType == CAN_INPUT_TYPE_FLOAT)
    {
        bspCanInputsReg[loc].rawValue = *((float*)(&bspCanInputsCfg[loc].rawData));
        bspCanInputsReg[loc].voltageValue = bspCanInputsReg[loc].rawValue; // Assume float is already in the 0-5000 mV range
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

bool BSP_CANIN_GetValueSchmitt(T_IN_CAN_LOC location)
{
    ASSERT(location < IN_CAN_MAX);
    return bspCanInputsReg[location].schmittState;
}

uint_fast16_t BSP_CANIN_GetValueAnalog(T_IN_CAN_LOC location)
{
    ASSERT(location < IN_CAN_MAX);
    return bspCanInputsReg[location].voltageValue;
}