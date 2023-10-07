#ifndef __LLGPIO_H_
#define __LLGPIO_H_

#include "stm32l496xx.h"
#include "typedefs.h"

typedef enum
{
    LL_GPIO_MODE_INPUT  = 0x00,
    LL_GPIO_MODE_OUTPUT = 0x01,
    LL_GPIO_MODE_AF     = 0x02,
    LL_GPIO_MODE_ANALOG = 0x03
}T_LLGPIO_MODE;

// STD Pin control

void LLGPIO_SetMode(T_IO io, T_LLGPIO_MODE mode);

void LLGPIO_SetStdState(T_IO io, bool state);

// PWM related pin control

void LLGPIO_InitPWM(T_IO io);

void LLGPIO_DeInitPWM(T_IO io);

void LLGPIO_SetDutyPWM(T_IO io, uint8_t duty);

#endif