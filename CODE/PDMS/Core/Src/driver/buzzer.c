#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "tim.h"
#include "cmsis_os2.h"

static T_IO buzzerIo =
{
    .pin = BUZZ_CTRL_Pin,
    .port = BUZZ_CTRL_GPIO_Port
};

void BUZZER_TurnOn()
{
    LL_GPIO_SetOutputPin(buzzerIo.port, buzzerIo.pin);
}

void BUZZER_TurnOff()
{
    LL_GPIO_ResetOutputPin(buzzerIo.port, buzzerIo.pin);
}