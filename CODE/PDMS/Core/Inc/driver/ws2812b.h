#include "main.h"

struct rgb_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct rgb_t rgb_t;

extern rgb_t red, green, blue, clear, rcpergol;

void WS2812B_Init(TIM_HandleTypeDef* htim, uint32_t Channel);
void WS2812B_Flush(void);
void WS2812B_SetSingle(uint16_t led_index, rgb_t color);
void WS2812B_Clear(void);
void WS2812_Test(void);
void WS2812B_StartupAction();