#ifndef __LLGPIO_H_
#define __LLGPIO_H_

#include "stm32l496xx.h"
#include "typedefs.h"

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

void BSP_OUT_SetMode(T_IO io, T_OUT_MODE mode);

void BSP_OUT_SetStdState(T_IO io, bool state);

// Batch Pin control

void BSP_OUT_SetBatchState(T_IO io, T_IO batchIo, bool state);

// PWM related pin control

void BSP_OUT_InitPWM(T_IO io);

void BSP_OUT_DeInitPWM(T_IO io);

void BSP_OUT_SetDutyPWM(T_IO io, uint8_t duty);

#endif