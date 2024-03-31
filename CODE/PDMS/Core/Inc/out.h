#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"
#include "spoc2.h"
#include "spoc2_definitions.h"

typedef enum
{
    OUT_TYPE_BTS500   = 0x00, // Simple high current switch Infineon BTS50010-1LUA
    OUT_TYPE_SPOC2    = 0x01, // Complex SPI based switch Infineon BTS72220-4ESA
}T_OUT_TYPE;

typedef struct _T_OUT_CFG
{
    const T_OUT_ID   id;         // id should reflect position in outsCfg
    const T_OUT_TYPE type;       // device type - can be BTS500 or BTS72220
    const T_SPOC2_ID spocId;     // If device is BTS72220 this holds sub device id
    const T_SPOC2_CH_ID spocChId; // If device is BTS72220 this holds sub device respecitve channel id
    T_OUT_MODE       mode;
    T_OUT_STATE      state;
    T_OUT_ID         batch; // optional for OUT_MODE_BATCH
    /* TODO ADD handling w safety*/
    bool             wsafety;
    uint8_t          octhreshold;
}T_OUT_CFG;

// Main outputs config
extern T_OUT_CFG outsCfg[];

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id );

void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode);

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE state);

bool OUT_ToggleState(T_OUT_ID id);

bool OUT_Batch(T_OUT_ID id, T_OUT_ID batch);

#endif