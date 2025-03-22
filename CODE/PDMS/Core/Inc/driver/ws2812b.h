#include "main.h"

struct rgb_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct rgb_t rgb_t;

extern rgb_t red, green, blue, clear, rcpergol;

void WS2812B_Init(void);
void WS2812B_StartupAction(void);
void WS2812B_DisableAll(void);