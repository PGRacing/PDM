#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "llgpio.h"

void LLGPIO_SetMode(T_IO io, T_LLGPIO_MODE mode)
{
    // io.gpio->MODER &= ~(3U << (io.pin * 2));        // Clear existing setting
    // io.gpio->MODER |= (mode & 3) << (io.pin * 2);   // Set new mode
}

void LLGPIO_SetStdState(T_IO io, bool state)
{
    HAL_GPIO_WritePin(io.gpio, io.pin, state);
}

void LLGPIO_InitPWM(T_IO io)
{
}

void LLGPIO_DeInitPWM(T_IO io)
{
}

void LLGPIO_SetDutyPWM(T_IO io, uint8_t duty)
{
}

