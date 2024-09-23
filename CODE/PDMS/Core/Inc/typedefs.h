#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

#include <stdbool.h>
#include <stdint.h>
#include "stm32l496xx.h"

#define FALSE false
#define TRUE  true

/// MACRO FUNCTIONS

/// Get array elements count
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

/// CONFIG
#define DISABLE_BUZZER 1
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

//ARM_CM_DWT
#define DEBUG_ARM_DWT_ENABLED

#ifdef DEBUG_ARM_DWT_ENABLED

#define  ARM_CM_DEMCR      (*(uint32_t *)0xE000EDFC)
#define  ARM_CM_DWT_CTRL   (*(uint32_t *)0xE0001000)
#define  ARM_CM_DWT_CYCCNT (*(uint32_t *)0xE0001004)


#define DEBUG_ARM_DWT_INIT if (ARM_CM_DWT_CTRL != 0) {      \
        ARM_CM_DEMCR      |= 1 << 24;  \
        ARM_CM_DWT_CYCCNT  = 0; \
        ARM_CM_DWT_CTRL   |= 1 << 0;   \
    }

#define DEBUG_ARM_GET_TIME ARM_CM_DWT_CYCCNT
                                        // CLOCK  // DIV
#define DEBUG_ARM_CLOCKS_TO_US(X) ((X) * 12.5) / 1000

#else

#define DEBUG_ARM_GET_TIME 0 
#define DEBUG_ARM_CLOCKS_TO_US(X) 0

#endif

#endif 