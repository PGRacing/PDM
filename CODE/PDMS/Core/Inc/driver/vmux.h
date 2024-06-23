#ifndef __VMUX_H_
#define __VMUX_H_

#include "stm32l496xx.h"

// Read voltage of one of 16 channels
uint32_t VMUX_GetValue(uint8_t index);

// Get battery voltage ADC channel 13
uint32_t VMUX_GetBattValue();

extern volatile uint16_t VMUX_LP1Voltage[4];
extern volatile uint16_t VMUX_LP2Voltage[4];

#endif