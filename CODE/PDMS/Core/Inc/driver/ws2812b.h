#ifndef __WS2812B_H_
#define __WS2812B_H_

#include "main.h"


/* This should be changed to driver and task moved elsewhere to ARGB module in APP*/
typedef struct _WS2812B_COLOR_T
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}WS2812B_COLOR_T;

void WS2812B_Init(void);
void WS2812B_StartupAction(void);
void WS2812B_DisableAll(void);

#endif