#ifndef __LLGPIO_H_
#define __LLGPIO_H_

#include "stm32l496xx.h"
#include "typedefs.h"

#define OUT_MAX 8
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
}T_OUT_ID;
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
}T_OUT_STATE;

// STD Pin control

void BSP_OUT_SetMode(T_OUT_ID id, T_OUT_MODE mode);

void BSP_OUT_SetStdState(T_OUT_ID id, bool state);

// Batch Pin control

void BSP_OUT_SetBatchState(T_OUT_ID id, T_OUT_ID batchId, bool state);

bool BSP_OUT_IsBatchPossible(T_OUT_ID id, T_OUT_ID batchId);

// PWM related pin control

void BSP_OUT_InitPWM(T_OUT_ID id);

void BSP_OUT_DeInitPWM(T_OUT_ID id);

void BSP_OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty);

#endif