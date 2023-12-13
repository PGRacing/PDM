#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"

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

typedef struct _T_OUT_CFG
{
    const T_OUT_ID id;    // id should reflect position in outsCfg
    const T_IO     io;
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

void OUT_ChangeMode(T_OUT_CFG* cfg, T_OUT_MODE targetMode);

bool OUT_SetState(T_OUT_CFG* cfg, T_OUT_STATE state);

bool OUT_ToggleState(T_OUT_CFG* cfg);

bool OUT_AttachAdditional(T_OUT_CFG* cfg, T_OUT_ID batch);

#endif