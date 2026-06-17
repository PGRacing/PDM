#ifndef __INPUT_H_
#define __INPUT_H_

#include "bsp_caninput.h"
#include "bsp_phyinput.h"

#define INPUT_ID_UNASSIGNED_VALUE 0xFFFF

typedef uint16_t T_INPUT_ID;

#pragma pack(push, 1)
typedef enum
{
    IN_MODE_UNUSED  = 0x00,
    IN_MODE_SCHMITT = 0x01,
    IN_MODE_ANALOG  = 0x02,
    IN_MODE_OUTPUT  = 0x03 // Change this IO pin to output
}T_IN_MODE;

typedef enum
{
    IN_TYPE_PHY      = 0x00, // When input is pure physical
    IN_TYPE_CAN      = 0x01, // When input data transmitted over can
    IN_TYPE_INVALID  = 0xFF, // Invalid input type
}T_IN_TYPE;

typedef struct _T_IN_CFG
{
    const uint16_t    location;
    T_IN_TYPE         type;
    T_IN_MODE         mode;
}T_IN_CFG;

extern T_IN_CFG inputsCfg[IN_PHY_MAX + IN_CAN_MAX];

#pragma pack(pop)

bool IN_GetValueSchmitt(uint16_t id);

uint32_t IN_GetValueAnalog(uint16_t id);

uint32_t IN_GetValue(T_INPUT_ID id);

T_IN_CFG *IN_GetCfgPtr(T_INPUT_ID id);

void IN_ChangeMode(T_IN_CFG* cfg, T_IN_MODE targetMode);

T_IN_MODE IN_GetMode(T_INPUT_ID id);

bool IN_OverrideCANValueSchmitt(T_INPUT_ID id, bool state);

#endif