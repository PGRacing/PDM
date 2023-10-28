#ifndef __INPUT_H_
#define __INPUT_H_

#include "typedefs.h"

typedef uint16_t T_INPUT_ID;
typedef enum
{
    IN_PHY_ID_1 = 0x00,
    IN_PHY_ID_2 = 0x01,
    IN_PHY_ID_3 = 0x02,
    IN_PHY_ID_4 = 0x03,
    IN_PHY_ID_5 = 0x04,
    IN_PHY_ID_6 = 0x05,
    IN_PHY_ID_7 = 0x06,
    IN_PHY_ID_8 = 0x07,
    IN_PHY_MAX
}T_IN_PHY_ID;

typedef enum
{
    IN_MODE_UNUSED  = 0x00,
    IN_MODE_SCHMITT = 0x01,
    IN_MODE_ANALOG  = 0x02,
    IN_MODE_OUTPUT  = 0x03, // Change this IO pin to output
}T_IN_MODE;

typedef enum
{
    IN_TYPE_PHY    = 0x00, // When input is pure physical
    IN_TYPE_CAN    = 0x01, // When input data transmitted over can
}T_IN_TYPE;

typedef struct _T_IN_CFG
{
    const T_INPUT_ID  id;      // id should reflect position in inputsCfg
    const T_IO        io;
    const uint32_t*   rawData; // pointer to rawData from input
    T_IN_TYPE         type;
    T_IN_MODE         mode;
}T_IN_CFG;

/* TODO If we want to use database structure and non static definition of inputs loaded from flash this should be created with dynamic - malloc */
extern T_IN_CFG inputsCfg[];

bool IN_GetValueSchmitt(uint16_t id);

uint32_t IN_GetValueAnalog(uint16_t id);

#endif