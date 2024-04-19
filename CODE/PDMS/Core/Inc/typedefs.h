#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

#include <stdbool.h>
#include <stdint.h>
#include "stm32l496xx.h"

#define FALSE false
#define TRUE  true

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

#define POWER_OUT_COUNT 16
#define USE_PDM_ASSERT 1

void assert_failed_pdm(uint8_t *file, uint32_t line);

#ifdef USE_PDM_ASSERT
#define ASSERT(expr) ((expr) ? (void)0U : assert_failed_pdm((uint8_t *)__FILE__, __LINE__))
#else
#define ASSERT(...)
#endif

typedef uint16_t T_INPUT_ID;

typedef enum
{
    STATUS_OK,
    STATUS_ERR,
}T_STATUS;

typedef struct _T_IO
{
    GPIO_TypeDef* port;
    uint16_t pin;
}T_IO;

typedef struct _T_CAN_IO
{
    uint32_t stdId;
    uint16_t offset;
}T_CAN_IO;

#endif 