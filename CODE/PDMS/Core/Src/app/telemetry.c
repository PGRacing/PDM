#include "telemetry.h"
#include "can_handler.h"
#include "out.h"
#include "pdm.h"
#include "vmux.h"
#include "string.h"
#include "cmsis_os2.h"
#include "semphr.h"

#define TELEM_OUT_NAME_PART 7

T_TELEM_CFG telemCfg = 
{
    .telemInterval = 50, // 50 ms interval
    .canInstance = CANH_INSTANCE_2,
    .sendSystemData = TRUE,
    .sendStatus = TRUE,
    .sendState = TRUE,
    .sendVoltage = TRUE,
    .sendCurrent = TRUE,
    .sendNames = TRUE,
};

static void TELEM_SendVoltageByCan(T_CANH_INSTANCE canInstance)
{
    CANH_Send_TxVoltage1_4(canInstance, OUT_DIAG_GetEmaVoltage(OUT_ID_1), OUT_DIAG_GetEmaVoltage(OUT_ID_2), OUT_DIAG_GetEmaVoltage(OUT_ID_3), OUT_DIAG_GetEmaVoltage(OUT_ID_4));
    CANH_Send_TxVoltage5_8(canInstance, OUT_DIAG_GetEmaVoltage(OUT_ID_5), OUT_DIAG_GetEmaVoltage(OUT_ID_6), OUT_DIAG_GetEmaVoltage(OUT_ID_7), OUT_DIAG_GetEmaVoltage(OUT_ID_8));
    CANH_Send_TxVoltage9_12(canInstance, OUT_DIAG_GetEmaVoltage(OUT_ID_9), OUT_DIAG_GetEmaVoltage(OUT_ID_10), OUT_DIAG_GetEmaVoltage(OUT_ID_11), OUT_DIAG_GetEmaVoltage(OUT_ID_12));
    CANH_Send_TxVoltage13_16(canInstance, OUT_DIAG_GetEmaVoltage(OUT_ID_13), OUT_DIAG_GetEmaVoltage(OUT_ID_14), OUT_DIAG_GetEmaVoltage(OUT_ID_15), OUT_DIAG_GetEmaVoltage(OUT_ID_16));
}

static void TELEM_SendCurrentByCan(T_CANH_INSTANCE canInstance)
{
    CANH_Send_TxCurrent1_4(canInstance, OUT_DIAG_GetEmaCurrent_pA(OUT_ID_1), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_2), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_3), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_4));
    CANH_Send_TxCurrent5_8(canInstance, OUT_DIAG_GetEmaCurrent_pA(OUT_ID_5), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_6), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_7), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_8));
    CANH_Send_TxCurrent9_12(canInstance, OUT_DIAG_GetEmaCurrent_pA(OUT_ID_9), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_10), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_11), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_12));
    CANH_Send_TxCurrent13_16(canInstance, OUT_DIAG_GetEmaCurrent_pA(OUT_ID_13), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_14), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_15), OUT_DIAG_GetEmaCurrent_pA(OUT_ID_16));
}

static void TELEM_SendStatusByCan(T_CANH_INSTANCE canInstance)
{
    CANH_Send_TxStatus1_8(canInstance, (uint8_t)OUT_DIAG_GetStatus(OUT_ID_1), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_2), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_3), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_4),
        (uint8_t)OUT_DIAG_GetStatus(OUT_ID_5), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_6), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_7), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_8));

    CANH_Send_TxStatus9_16(canInstance, (uint8_t)OUT_DIAG_GetStatus(OUT_ID_9), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_10), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_11), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_12),
        (uint8_t)OUT_DIAG_GetStatus(OUT_ID_13), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_14), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_15), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_16));
}

static void TELEM_SendSystemDataByCan(T_CANH_INSTANCE canInstance)
{
    CANH_Send_SysStatus(canInstance, (uint8_t)PDM_GetSysStatus(), (uint16_t)VMUX_GetBattValueEma(), (int16_t)VMUX_GetTempValue(), (uint8_t)PDM_GetSafetyState());
}

static void TELEM_SendStateByCan(T_CANH_INSTANCE canInstance)
{
    uint8_t dummyStateArr[OUT_ID_MAX] = {0x00};
    for(T_OUT_ID id = 0 ; id < OUT_ID_MAX; id++)
    {
        dummyStateArr[id] = OUT_DIAG_GetState(id);
    }

    CANH_Send_TxState1_16(canInstance, dummyStateArr);
}

static void TELEM_SendNamesByCan(T_CANH_INSTANCE canInstance)
{
    for(T_OUT_ID id = 0 ; id < OUT_ID_MAX; id++)
    {
        char* name = OUT_GetName(id);
        size_t namelen = strlen(name);
        if( namelen != 0)
        {
            uint8_t parts = (namelen / TELEM_OUT_NAME_PART) + 1;
            for(uint8_t i = 0; i < parts; i++)
            {
                CANH_Send_Names(canInstance, id, i, (name + i * TELEM_OUT_NAME_PART));
            }
        }
    }
}

void telemTaskStart(void *argument)
{
    LOG_INFO("TELEM:: Task start");
    uint8_t iOffsetCounter = 0;
    const uint8_t iOffset = 5;

    for (;;)
    {
        if (telemCfg.sendStatus == TRUE)
        {
            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendStatusByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendStatusByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendStatusByCan(CANH_INSTANCE_1);
                TELEM_SendStatusByCan(CANH_INSTANCE_2);
            }
        }

        if (telemCfg.sendState == TRUE)
        {

            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendStateByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendStateByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendStateByCan(CANH_INSTANCE_1);
                TELEM_SendStateByCan(CANH_INSTANCE_2);
            }
        }

        if (telemCfg.sendVoltage == TRUE)
        {
            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendVoltageByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendVoltageByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendVoltageByCan(CANH_INSTANCE_1);
                TELEM_SendVoltageByCan(CANH_INSTANCE_2);
            }
        }

        if (telemCfg.sendCurrent == TRUE)
        {
            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendCurrentByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendCurrentByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendCurrentByCan(CANH_INSTANCE_1);
                TELEM_SendCurrentByCan(CANH_INSTANCE_2);
            }
        }

        if (telemCfg.sendSystemData == TRUE)
        {
            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendSystemDataByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendSystemDataByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendSystemDataByCan(CANH_INSTANCE_1);
                TELEM_SendSystemDataByCan(CANH_INSTANCE_2);
            }
        }

        osDelay(pdMS_TO_TICKS(telemCfg.telemInterval));

        if (iOffsetCounter == iOffset && telemCfg.sendNames == TRUE)
        {
            if (telemCfg.canInstance == CANH_INSTANCE_1)
            {
                TELEM_SendNamesByCan(CANH_INSTANCE_1);
            }
            else if (telemCfg.canInstance == CANH_INSTANCE_2)
            {
                TELEM_SendNamesByCan(CANH_INSTANCE_2);
            }
            else
            {
                TELEM_SendNamesByCan(CANH_INSTANCE_1);
                TELEM_SendNamesByCan(CANH_INSTANCE_2);
            }
        }
        iOffsetCounter++;
    }
}

/// 
/// TODO [LOW] Add memory and on-board temperatures to telemetry & verify output speed (can bus)
///