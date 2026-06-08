#ifndef __TELEMETRY_H_
#define __TELEMETRY_H_

#include "typedefs.h"
#include "can_handler.h"

/// @brief Telemetry configuration struct
typedef struct _T_TELEM_CFG
{
    uint16_t          telemInterval;     // Telemetry reporting interval in ms
    BaseType_t        namesInterval;     // Names reporting interval in ticks
    T_CANH_INSTANCE   canInstance;       // Reporting can instance (if 0x02 selected then both instances will be used)
    bool              sendSystemData;    // Flag to send system data
    bool              sendStatus;        // Flag to send status information
    bool              sendState;         // Flag to send state information
    bool              sendVoltage;       // Flag to send voltage data
    bool              sendCurrent;       // Flag to send current data
    bool              sendNames;         // Flag to send names
    bool              sendPhyInputs;     // Flag to send physical inputs data
    bool              sendImu;     // Flag to send physical inputs data
} T_TELEM_CFG; 

/// @brief Telemetry module configuration
extern T_TELEM_CFG telemCfg;

void telemTaskStart(void *argument);

#endif