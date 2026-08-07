#ifndef __PDM_H_
#define __PDM_H_

#include "typedefs.h"
#include "logger.h"
#include "semphr.h"
#include "cmsis_os2.h"

/* Modules includes */
#include "typedefs.h"
#include "features.h"
#include "logger.h"
#include "input.h"
#include "logic.h"
#include "out.h"
#include "vmux.h"
#include "adc_handler.h"
#include "can_handler.h"
#include "telemetry.h"
#include "buzzer.h"
#include "ws2812b.h"
#include "spoc2.h"

typedef enum
{
    PDM_SYS_STATUS_OK = 0,    // System ok
    PDM_SYS_STATUS_ERROR = 1, // Unknown error
    PDM_SYS_STATUS_UVLO = 2,  // Under voltage lockout mode
}T_PDM_SYS_STATUS;

typedef struct __packed
{
    uint32_t      minBattVolage; // minimal battery voltage that the platform will start [mV]
    uint32_t      onInitBattCheckMaxTries; // maximal number that battery voltage will be checked on startup
    bool          isUvloEnabled;  // UVLO if this field is set for true, uvlo protection is enabled
    uint32_t      uvloVoltageLoThreshold; // UVLO protection minimal voltage threshold, should always be higher than minBattVoltage [mV]
    uint32_t      uvloVoltageHiThreshold; // UVLO protection minimal voltage threshold, should always be higher than uvloVoltageLoThreshold [mV]
    uint32_t      uvloTimeThreshold; // UVLO protection time threshold in full 100's [ms]
    uint8_t       uvloRetainDivider; // UVLO time threshold is divided by this value, this gives time in [ms] where voltage above uvloVoltageHiThreshold allows to retain to normal operation
    
    bool          useBuzzer; // use buzzer on device
    uint8_t       ledMode;   // Selected led mode
    
    // Pointers to modules configuration
    T_OUT_CFG    (*pOutsCfg)[OUT_ID_MAX];        // Output channels configuration
    T_IN_CFG     (*pInsCfg)[];                   // Input channels configuration
    T_LOGIC_CFG  (*pLogicCfg)[POWER_OUT_COUNT];  // Logic configuration
    T_TELEM_CFG*  pTelemCfg;                     // Telemetry configuration
    T_CANH_CFG   (*pCansCfg)[CANH_INSTANCE_MAX]; // CAN configuration

}T_PDM_CFG;

typedef struct 
{
    T_PDM_SYS_STATUS status;
    bool             safetyState;
    bool             uvloAssessment;  // Is UVLO assesment in progress?
    uint32_t         uvloLoCounter;   // Undervoltage LO protection counter
    uint32_t         uvloHiCounter;   // Undervoltage HI protection counter
    osTimerId_t      uvloTimer;       // Undervoltage protection timer
    osTimerId_t      softLimpTimer;   // Soft limp home mode timer
    uint32_t         resetGwCounter;  // Counter for reset gateway asks
    uint8_t          rtosSysLoad;     // System load in percentage (0-100) from RTOS idle task runtime counter    
}T_PDM_REG;


extern volatile T_PDM_CFG pdmCfg;
extern volatile T_PDM_REG pdmReg;

T_PDM_SYS_STATUS PDM_GetSysStatus(void);
uint8_t PDM_GetRtosSysLoad(void);

bool PDM_GetSafetyState(void);
void PDM_CANResetGateway(uint32_t id, uint8_t* data, uint32_t size);
// Platform start
void PDM_Init(void);

#endif