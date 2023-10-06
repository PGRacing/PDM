#include "typedefs.h"

//BSP
#include "stm32l4xx_hal_tim.h"

#define MAX_PWM_OUTPUTS 8

typedef struct
{
    TIM_TypeDef* tim;
    io_t io;
}pwmConfig_t;

pwmConfig_t pwmConfigArray[MAX_PWM_OUTPUTS] =
        {
                {
                    .tim = TIM3,
                    .io = { GPIOC, GPIO_PIN_9}
                }

        };

HAL_StatusTypeDef prepareTimer( TIM_TypeDef* tim)
{
//    RCC_APB2ENR_TIM3EN
//    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;    // Enable TIM11 peripheral clock source
//    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;    // Enable GPIOC peripheral clock source
}

HAL_StatusTypeDef pwmInitialize(pwmConfig_t config)
{

}