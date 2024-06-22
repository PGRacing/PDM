#include "telemetry.h"
#include "can_handler.h"
#include "out.h"
#include "pdm.h"
#include "vmux.h"
#include "cmsis_os2.h"


static void TELEM_SendVoltageByCan()
{
    // All casting form uint32_t to uint16_t is intentional
    CANH_Send_TxVoltage1_4(   OUT_DIAG_GetVoltage(OUT_ID_1), OUT_DIAG_GetVoltage(OUT_ID_2), OUT_DIAG_GetVoltage(OUT_ID_3), OUT_DIAG_GetVoltage(OUT_ID_4));
    CANH_Send_TxVoltage5_8(   OUT_DIAG_GetVoltage(OUT_ID_5), OUT_DIAG_GetVoltage(OUT_ID_6), OUT_DIAG_GetVoltage(OUT_ID_7), OUT_DIAG_GetVoltage(OUT_ID_8));
    CANH_Send_TxVoltage9_12(  OUT_DIAG_GetVoltage(OUT_ID_9), OUT_DIAG_GetVoltage(OUT_ID_10), OUT_DIAG_GetVoltage(OUT_ID_11), OUT_DIAG_GetVoltage(OUT_ID_12));
    CANH_Send_TxVoltage13_16( OUT_DIAG_GetVoltage(OUT_ID_13), OUT_DIAG_GetVoltage(OUT_ID_14), OUT_DIAG_GetVoltage(OUT_ID_15), OUT_DIAG_GetVoltage(OUT_ID_16));
}

static void TELEM_SendCurrentByCan()
{
    CANH_Send_TxCurrent1_4(   OUT_DIAG_GetCurrent_pA(OUT_ID_1), OUT_DIAG_GetCurrent_pA(OUT_ID_2), OUT_DIAG_GetCurrent_pA(OUT_ID_3), OUT_DIAG_GetCurrent_pA(OUT_ID_4));
    CANH_Send_TxCurrent5_8(   OUT_DIAG_GetCurrent_pA(OUT_ID_5), OUT_DIAG_GetCurrent_pA(OUT_ID_6), OUT_DIAG_GetCurrent_pA(OUT_ID_7), OUT_DIAG_GetCurrent_pA(OUT_ID_8));
    CANH_Send_TxCurrent9_12(  OUT_DIAG_GetCurrent_pA(OUT_ID_9), OUT_DIAG_GetCurrent_pA(OUT_ID_10), OUT_DIAG_GetCurrent_pA(OUT_ID_11), OUT_DIAG_GetCurrent_pA(OUT_ID_12));
    CANH_Send_TxCurrent13_16( OUT_DIAG_GetCurrent_pA(OUT_ID_13), OUT_DIAG_GetCurrent_pA(OUT_ID_14), OUT_DIAG_GetCurrent_pA(OUT_ID_15), OUT_DIAG_GetCurrent_pA(OUT_ID_16));
}

static void TELEM_SendStatusByCan()
{
    CANH_Send_TxStatus1_8((uint8_t)OUT_DIAG_GetStatus(OUT_ID_1), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_2), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_3), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_4),
        (uint8_t)OUT_DIAG_GetStatus(OUT_ID_5), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_6), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_7), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_8));

    CANH_Send_TxStatus9_16((uint8_t)OUT_DIAG_GetStatus(OUT_ID_9), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_10), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_11), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_12),
        (uint8_t)OUT_DIAG_GetStatus(OUT_ID_13), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_14), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_15), (uint8_t)OUT_DIAG_GetStatus(OUT_ID_16));
}

static void TELEM_SendSystemDataByCan()
{
    CANH_Send_SysStatus((uint8_t)PDM_GetSysStatus(), (uint16_t)VMUX_GetBattValue());
}

static void TELEM_SendStateByCan()
{
    uint8_t dummyStateArr[OUT_ID_MAX] = {0x00};
    for(T_OUT_ID id = 0 ; id < OUT_ID_MAX; id++)
    {
        dummyStateArr[id] = OUT_DIAG_GetState(id);
    }

    CANH_Send_TxState1_16(dummyStateArr);
}

void telemTaskStart(void *argument)
{
    /* USER CODE BEGIN telemTaskStart */
    /* Infinite loop */
    for(;;)
    {   
        TELEM_SendStatusByCan();
        TELEM_SendStateByCan();
        TELEM_SendVoltageByCan();
        TELEM_SendCurrentByCan();
        TELEM_SendSystemDataByCan();
        osDelay(pdMS_TO_TICKS(50));
    }
    /* USER CODE END telemTaskStart */
}