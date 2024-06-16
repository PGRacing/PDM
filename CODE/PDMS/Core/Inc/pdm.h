#ifndef __PDM_H_
#define __PDM_H_

typedef enum
{
    PDM_SYS_STATUS_OK = 0,
    PDM_SYS_STATUS_ERROR = 1,
}T_PDM_SYS_STATUS;

T_PDM_SYS_STATUS PDM_GetSysStatus();

#endif