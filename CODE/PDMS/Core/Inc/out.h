#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"

typedef struct _T_OUT_CFG
{
    const T_OUT_ID id;    // id should reflect position in outsCfg
    T_OUT_MODE     mode;
    T_OUT_STATE    state;
    T_OUT_ID       batch; // optional for OUT_MODE_BATCH
    /* TODO ADD handling w safety*/
    bool           wsafety;
    uint8_t        octhreshold;
}T_OUT_CFG;

// Main outputs config
extern T_OUT_CFG outsCfg[];

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id );

void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode);

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE state);

bool OUT_ToggleState(T_OUT_ID id);

bool OUT_Batch(T_OUT_ID id, T_OUT_ID batch);

#endif