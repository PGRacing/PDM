#ifndef __BSP_OUT_H_
#define __BSP_OUT_H_

#include "stm32l496xx.h"
#include "typedefs.h"

// Max index of BTS500 type device
#define OUT_ID_BTS_MAX 8

/// @brief Output channels ID
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

/// @brief Output channel mode
typedef enum
{
    OUT_MODE_UNUSED   = 0x00, // Unused - external control
    OUT_MODE_STD      = 0x01, // Standard control mode
    OUT_MODE_PWM      = 0x02, // PWM output for load balancing
    OUT_MODE_BATCH    = 0x03  // Batching two inputs works for std
}T_OUT_MODE;

/// @brief Output channel state
typedef enum
{
    OUT_STATE_OFF     = 0x00,
    OUT_STATE_ON      = 0x01,
    OUT_STATE_ERR_LATCH   = 0x02
}T_OUT_STATE;

// TODO Consider changing names is S_AND_H_OC used
/// @brief Output channel status
typedef enum
{
    OUT_STATUS_OK            = 0,
    OUT_STATUS_SOFT_OC       = 1,
    OUT_STATUS_HARD_OC_OR_OT = 2,
    OUT_STATUS_SHORT_TO_VSS  = 3,
    OUT_STATUS_OPEN_LOAD     = 4,
    OUT_STATUS_S_AND_H_OC    = 5,
    OUT_STATUS_CONTROL_FAIL  = 6,
}T_OUT_STATUS;

// STD PIN CONTROL

/// @brief Initialize output channel control pin used mode
/// @param id Output channel id [1..16] T_OUT_ID
/// @param targetMode Target mode [UNUSED/STD/PWM/BATCH]
void BSP_OUT_SetMode(T_OUT_ID id, T_OUT_MODE targetMode);

/// @brief Set output channel control pin state
/// @param id Output channel id [1..16] T_OUT_ID
/// @param reqState Requested output channel state [ON/OFF]
void BSP_OUT_SetStdState(T_OUT_ID id, bool reqState);

// BATCH PIN CONTROL

/// @brief Set output channel and batch channel control pin state
/// @param id Output channel id [1..16] T_OUT_ID
/// @param batchId Output batch channel id [1..16] T_OUT_ID
/// @param reqState Requested output channel's state [ON/OFF]
void BSP_OUT_SetBatchState(T_OUT_ID id, T_OUT_ID batchId, bool reqState);

/// @brief Toggle [on/off] state of selected output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @return FALSE if error occured, TRUE if ok

/// @brief Check if batch between two output channels is possible
/// @param id Output channel id [1..16] T_OUT_ID
/// @param batchId Output batch channel id [1..16] T_OUT_ID
/// @return FALSE if batch not possible, TRUE if possible
bool BSP_OUT_IsBatchPossible(T_OUT_ID id, T_OUT_ID batchId);

// PWM PIN CONTROL

/// @brief  Set output channel control pin PWM signal duty
/// @param id Output channel id [1..16] T_OUT_ID
/// @param duty PWM signal duty 0 - 100%
void BSP_OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty);

/// @brief Initialize output channel control pin to PWM state
/// @param id Output channel id [1..16] T_OUT_ID
void BSP_OUT_InitPWM(T_OUT_ID id);

/// @brief Deinitialize output channel control pin from PWM state
/// @param id Output channel id [1..16] T_OUT_ID
void BSP_OUT_DeInitPWM(T_OUT_ID id);

// PIN GET DATA

/// @brief Get value of ADC readout from I(sense) pin - raw data
/// @param id Output channel id [1..16] T_OUT_ID
/// @return ADC readout [0..4096]
uint32_t BSP_OUT_GetCurrentAdcValue(T_OUT_ID id);   

/// @brief Get Dkilis parameter of output channel - current divider for Is pin
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Dkilis absolute value Dkilis = I(load)/I(sense)
uint32_t BSP_OUT_GetDkilis(T_OUT_ID id);

/// @brief Get fault level parameter of output channel - when I(load) current is considered as fault state [mA]
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Fault level [mA]
uint32_t BSP_OUT_GetFaultLevel(T_OUT_ID id);

/// @brief Calculate load current of output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output current in [mA]
uint32_t BSP_OUT_CalcCurrent(T_OUT_ID id);

#endif