#ifndef __TMP126_H_
#define __TMP126_H_

#include "typedefs.h"

#include "stm32l4xx_hal.h"
#include "spi.h"

int TMP126_GetID();

/// @brief Get current temperature value read from sensor, updated in tmp126TaskEntry
/// @param  
/// @return temperature as float
float TMP126_GetTemp(void);

/// @brief Get current temperature value read from sensor, updated in tmp126TaskEntry
/// @param  
/// @return temperature as int16_t, value is temperature multiplied by 10 (e.g 253 means 25.3 C)
int16_t TMP126_GetTempInt(void);

#endif