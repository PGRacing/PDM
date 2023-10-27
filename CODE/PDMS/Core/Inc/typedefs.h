#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

#include <stdbool.h>
#include <stdint.h>

#define FALSE false
#define TRUE  true

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

#define POWER_OUT_COUNT 16

#define USE_FULL_ASSERT 1
#define ASSERT(...) assert_param(__VA_ARGS__)

typedef enum
{
    STATUS_OK,
    STATUS_ERR,
}T_STATUS;

typedef struct _T_IO
{
    GPIO_TypeDef* gpio;
    uint16_t pin;
}T_IO;

#endif 