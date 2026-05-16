#ifndef __BSP_CANINPUT_H_
#define __BSP_CANINPUT_H_

#include "can_handler.h"

typedef enum
{
    IN_CAN_LOC_1 = 0x00,
    IN_CAN_LOC_2 = 0x01,
    IN_CAN_LOC_3 = 0x02,
    IN_CAN_LOC_4 = 0x03,
    IN_CAN_LOC_5 = 0x04,
    IN_CAN_LOC_6 = 0x05,
    IN_CAN_LOC_7 = 0x06,
    IN_CAN_LOC_8 = 0x07,
    IN_CAN_LOC_9 = 0x08,
    IN_CAN_LOC_10 = 0x09,
    IN_CAN_LOC_11 = 0x0A,
    IN_CAN_LOC_12 = 0x0B,
    IN_CAN_LOC_13 = 0x0C,
    IN_CAN_LOC_14 = 0x0D,
    IN_CAN_LOC_15 = 0x0E,
    IN_CAN_LOC_16 = 0x0F,
    IN_CAN_MAX
}T_IN_CAN_LOC;

typedef enum _T_BSP_CANIN_DATATYPE
{
    CAN_INPUT_TYPE_BOOL,
    CAN_INPUT_TYPE_UINT16,
    CAN_INPUT_TYPE_UINT32,
    CAN_INPUT_TYPE_INT16,
    CAN_INPUT_TYPE_INT32,
    CAN_INPUT_TYPE_FLOAT,
}T_BSP_CANIN_DATATYPE;

typedef struct _T_BSP_CANIN_CFG
{
    bool isUsed;
    T_CANH_INSTANCE canInstance;
    uint32_t canId;
    uint16_t offset;
    T_BSP_CANIN_DATATYPE dataType;
    uint32_t rawData;
}T_BSP_CANIN_CFG;

void BSP_CANIN_DispatchFrame(T_CANH_INSTANCE canInstance, uint32_t id, uint8_t* raw, uint8_t dlc);
bool BSP_CANIN_GetValueSchmitt(T_IN_CAN_LOC location);
uint_fast16_t BSP_CANIN_GetValueAnalog(T_IN_CAN_LOC location);


#endif // __BSP_CANINPUT_H_