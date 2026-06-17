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

#pragma pack(push, 1)
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
}T_BSP_CANIN_CFG;

#pragma pack(pop)

/// @brief Dispatch a received CAN frame to the appropriate input based on its ID and instance
/// @param canInstance The CAN instance to which the frame was received
/// @param id The ID of the received CAN frame
/// @param raw A pointer to the raw data of the received CAN frame
/// @param dlc The data length code of the received CAN frame
/// @note This function should be called from CAN receive callback after receiving a CAN frame
void BSP_CANIN_DispatchFrame(T_CANH_INSTANCE canInstance, uint32_t id, uint8_t* raw, uint8_t dlc);

/// @brief Get CAN input value as schmitt trigger
/// @param location location of the CAN input
/// @return Input value as schmitt trigger
/// @note This function should be called only for CAN inputs
bool BSP_CANIN_GetValueSchmitt(T_IN_CAN_LOC location);

/// @brief Get CAN input value as analog
/// @param location location of the CAN input
/// @return Input value as analog
/// @note This function should be called only for CAN inputs
uint_fast16_t BSP_CANIN_GetValueAnalog(T_IN_CAN_LOC location);

/// @brief Override CAN input state (after usage of previous value)
/// @param location The location of the CAN input
/// @param state The new state to set
/// @return TRUE if the override was successful, FALSE otherwise
bool BSP_CANIN_OverrideValueSchmitt(T_IN_CAN_LOC location, bool state);

extern T_BSP_CANIN_CFG bspCanInputsCfg[IN_CAN_MAX];

#endif // __BSP_CANINPUT_H_