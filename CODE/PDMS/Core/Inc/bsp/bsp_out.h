#ifndef __BSP_OUT_H_
#define __BSP_OUT_H_

#include "stm32l496xx.h"
#include "typedefs.h"

typedef enum
{
    OUT_ID_1 = 0x00,
    OUT_ID_2 = 0x01,
    OUT_ID_3 = 0x02,
    OUT_ID_4 = 0x03,
    OUT_ID_5 = 0x04,
    OUT_ID_6 = 0x05,
    OUT_ID_7 = 0x06,
    OUT_ID_8 = 0x07,
    OUT_ID_9 = 0x08,
    OUT_ID_10 = 0x09,
    OUT_ID_11 = 0x0A,
    OUT_ID_12 = 0x0B,
    OUT_ID_13 = 0x0C,
    OUT_ID_14 = 0x0D,
    OUT_ID_15 = 0x0E,
    OUT_ID_16 = 0x0F,
    OUT_ID_MAX
}T_OUT_ID;

#define OUT_ID_BTS_MAX 8
typedef enum
{
    OUT_MODE_UNUSED   = 0x00, // Unused - external control
    OUT_MODE_STD      = 0x01, // Standard control mode
    OUT_MODE_PWM      = 0x02, // PWM output for load balancing
    OUT_MODE_BATCH    = 0x03  // Batching two inputs works for std
}T_OUT_MODE;

typedef enum
{
    OUT_STATE_OFF     = 0x00,
    OUT_STATE_ON      = 0x01,
    OUT_STATE_ERR_LATCH   = 0x02
}T_OUT_STATE;

typedef enum
{
    OUT_STATUS_NORMAL_OFF    = 0,
    OUT_STATUS_NORMAL_ON     = 1,
    OUT_STATUS_SOFT_OC       = 2,
    OUT_STATUS_HARD_OC_OR_OT = 3,
    OUT_STATUS_SHORT_TO_VSS  = 4,
    OUT_STATUS_OPEN_LOAD     = 5,
    OUT_STATUS_S_AND_H_OC    = 6,
    OUT_STATUS_ERR_LATCH     = 8
}T_OUT_STATUS;

// STD Pin control

void BSP_OUT_SetMode(T_OUT_ID id, T_OUT_MODE mode);

void BSP_OUT_SetStdState(T_OUT_ID id, bool state);

// Batch Pin control

void BSP_OUT_SetBatchState(T_OUT_ID id, T_OUT_ID batchId, bool state);

bool BSP_OUT_IsBatchPossible(T_OUT_ID id, T_OUT_ID batchId);

// PWM related pin control

void BSP_OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty);

void BSP_OUT_InitPWM(T_OUT_ID id);

void BSP_OUT_DeInitPWM(T_OUT_ID id);

uint32_t BSP_OUT_GetVoltageAdcValue(T_OUT_ID id);

uint32_t BSP_OUT_GetCurrentAdcValue(T_OUT_ID id);   

uint32_t BSP_OUT_GetDkilis(T_OUT_ID id);

uint32_t BSP_OUT_CalcCurrent(T_OUT_ID id);

#endif