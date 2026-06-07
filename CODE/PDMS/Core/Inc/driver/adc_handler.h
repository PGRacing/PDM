#ifndef __ADC_HANDLER_H_
#define __ADC_HANDLER_H_

#include "adc.h"

extern volatile uint16_t adc1AvgData[ADC1_CHANNEL_COUNT];

void ADCH_Init(void);

#endif