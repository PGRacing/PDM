#ifndef __BSP_CMNINPUT_H_
#define __BSP_CMNINPUT_H_

#include "typedefs.h"

#define IN_SCHMITT_HIGH_THRESHOLD 2500 // [mV] Above this value input is considered HIGH
#define IN_SCHMITT_LOW_THRESHOLD 1500 // [mV] Below this value input is considered LOW

/// @brief Common input register structure used for both physical and CAN inputs
typedef struct _T_BSP_IN_REG
{
   uint_fast32_t rawValue;
   bool          schmittState;
   uint_fast16_t voltageValue; // 0 - 5000 mV voltage range
   uint_fast16_t emaVoltageValue; // EMA filtered voltage value 0 - 5000 mV range
}
T_BSP_IN_REG;

#endif