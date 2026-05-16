#ifndef __BSP_CMNINPUT_H_
#define __BSP_CMNINPUT_H_

#include "typedefs.h"

#define IN_SCHMITT_HIGH_THRESHOLD 2500
#define IN_SCHMITT_LOW_THRESHOLD 1500

typedef struct _T_BSP_IN_REG
{
   uint_fast16_t rawValue;
   bool          schmittState;
   uint_fast16_t voltageValue; // 0 - 5000 mV voltage range
}
T_BSP_IN_REG;

#endif