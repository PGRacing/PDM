#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"

typedef enum
{
    OUT_ID_1 = 0x00,
    OUT_ID_2 = 0x01,
    OUT_ID_3 = 0x02,
    OUT_ID_4 = 0x03,
    OUT_ID_5 = 0x04,
    OUT_ID_6 = 0x05,
    OUT_ID_7 = 0x06,
    OUT_ID_8 = 0x07,
}T_OUT_ID;

typedef enum
{
    OUT_MODE_UNUSED   = 0x00, // Unused - external control
    OUT_MODE_STD      = 0x01, // Standard control mode
    OUT_MODE_PWM      = 0x02, // PWM output for load balancing
    OUT_MODE_BATCH    = 0x03  // Batching two inputs works for std
}T_OUT_MODE;

typedef enum
{
    OUT_STATE_OFF     = 0x00,
    OUT_STATE_ON      = 0x01,
}T_OUT_STATE;

typedef struct _T_OUT_CFG
{
    const T_OUT_ID id;    // id should reflect position in outsCfg
    const T_IO     io;
    T_OUT_MODE     mode;
    T_OUT_STATE    state;
    T_OUT_ID       batch; // optional for OUT_MODE_BATCH
}T_OUT_CFG;

// Main outputs config
extern T_OUT_CFG outsCfg[];

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id );

void OUT_ChangeMode(T_OUT_CFG* cfg, T_OUT_MODE targetMode);

bool OUT_SetState(T_OUT_CFG* cfg, T_OUT_STATE state);

bool OUT_AttachAdditional(T_OUT_CFG* cfg, T_OUT_ID batch);

#endif