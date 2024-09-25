#ifndef __PDM_H_
#define __PDM_H_

#include "typedefs.h"

typedef enum
{
    PDM_SYS_STATUS_OK = 0,
    PDM_SYS_STATUS_ERROR = 1,
}T_PDM_SYS_STATUS;

typedef struct 
{
    T_PDM_SYS_STATUS status;
    bool             safetyState;
}T_PDM_ALL;

extern volatile T_PDM_ALL pdmAll;

T_PDM_SYS_STATUS PDM_GetSysStatus(void);
bool PDM_GetSafetyState(void);

#endif