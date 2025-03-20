#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_gpio.h"
#include "tim.h"
#include "cmsis_os2.h"

#ifdef USE_BUZZER
static T_IO buzzerIo =
{
    .pin = BUZZ_CTRL_Pin,
    .port = BUZZ_CTRL_GPIO_Port
};
#endif

void BUZZER_TurnOn(void)
{
#ifdef USE_BUZZER
    LL_GPIO_SetOutputPin(buzzerIo.port, buzzerIo.pin);
#endif
}

void BUZZER_TurnOff(void)
{
#ifdef USE_BUZZER
    LL_GPIO_ResetOutputPin(buzzerIo.port, buzzerIo.pin);
#endif
}