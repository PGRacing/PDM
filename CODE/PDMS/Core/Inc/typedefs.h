#ifndef __TYPEDEFS_H__
#define _TYPEDEFS_H__
#include "stdbool.h"
#include "gpio.h"

typedef struct
{
    GPIO_TypeDef* port;
    uint16_t pin;
}io_t;

#endif