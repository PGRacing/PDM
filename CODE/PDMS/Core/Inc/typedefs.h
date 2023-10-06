#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

#include <stdbool.h>
#include <stdint.h>

#define FALSE false
#define TRUE  true

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

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