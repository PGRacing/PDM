#ifndef __BSP_SPOC2_H_
#define __BSP_SPOC2_H_

#include "typedefs.h"
#include "spoc2.h"
#include "spoc2_definitions.h"

bool BSP_SPOC2_TransferBlocking(T_SPOC2_ID id, uint8_t* txBuffer, uint8_t* rxBuffer, uint32_t dataLen);

#endif