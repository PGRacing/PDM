#ifndef __ISOTP_APP_H_
#define __ISOTP_APP_H_

#include "typedefs.h"

void APP_ISOTP_Init(void);
void APP_ISOTP_EntranceGateway(uint32_t id, uint8_t* data, uint32_t size);
void APP_ISOTP_DispatchFrame(uint8_t*data, uint32_t dlc);

#endif