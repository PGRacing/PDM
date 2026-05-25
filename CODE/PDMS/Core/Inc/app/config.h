#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "typedefs.h"

typedef enum
{
    CONFIG_CAN_ID_ENTER = 0x490,
    CONFIG_CAN_ID_TX = 0x491,
    CONFIG_CAN_ID_RX = 0x492,
}T_CONFIG_CAN_ID;


typedef enum
{
    CONFIG_SELECTION_A = 0x00,
    CONFIG_SELECTION_B = 0x01,
    CONFIG_SELECTION_MAX
} T_CONFIG_SELECTION;

void CONFIG_LoadConfig(T_CONFIG_SELECTION configSelection);
void CONFIG_NewConfig(T_CONFIG_SELECTION configSelection, uint8_t* data, uint32_t size);
void CONFIG_GetCfgPtr(T_CONFIG_SELECTION configSelection, uint8_t** outPtr);
void CONFIG_GetCfgExpSize(uint32_t* outSize);

#endif // __CONFIG_H__