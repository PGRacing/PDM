#include "main.h"

struct rgb_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct rgb_t rgb_t;

extern rgb_t red, green, blue, clear, rcpergol;

void ws2812b_init(TIM_HandleTypeDef* htim, uint32_t Channel);
void ws2812b_flush(void);
void ws2812b_set_single(uint16_t led_index, rgb_t color);
void ws2812b_clear(void);
void ws2812b_test(void);