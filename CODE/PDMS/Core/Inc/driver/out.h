#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"
#include "spoc2.h"
#include "spoc2_definitions.h"
#include "cmsis_os2.h"

typedef enum
{
    OUT_TYPE_BTS500   = 0x00, // Simple high current switch Infineon BTS50010-1LUA
    OUT_TYPE_SPOC2    = 0x01, // Complex SPI based switch Infineon BTS72220-4ESA
}T_OUT_TYPE;

typedef enum
{
    OUT_ERR_BEH_NO         = 0x00,  // In case of error nothing happens
    OUT_ERR_BEH_LATCH      = 0x01,  // In case of error output is latched till channel reset
    OUT_ERR_BEH_TIME_LATCH = 0x03,  // In case of error output is latched for give amount of time
    OUT_ERR_BEH_TRY_RETRY  = 0x04,  // In case of error output there are few retries before latch
}T_OUT_ERR_BEHAVIOR;

// After error configuration
typedef struct _T_OUT_SAFETY_AERR_CFG
{
    T_OUT_ERR_BEHAVIOR behavior;
    uint32_t latchTime; // Time for /ref OUT_ERR_BEH_TIME_LATCH in ms
}T_OUT_SAFETY_AERR_CFG;

#define OUT_SAFETY_UNLIMITED_RETIRES 0xFFFF
#define OUT_DIAG_NAME_LEN 32

typedef struct _T_OUT_SAFETY_CFG
{
    T_OUT_SAFETY_AERR_CFG aerrCfg;  // Type beahavior if error occurs
    bool               actOnSafety; // This specifies if this channel should be turned of when "safety line" is opened
    bool               useOc;
    uint32_t           ocThreshold;     // Over-current treshold in mili-ampers
    uint16_t           ocTripCounter;    // Counter of ms to trip of overcurrent
    const uint16_t     ocTripThreshold;  // Number of ms to trip of overcurrent
    uint16_t           errRetryCounter;  // Counter of performed retries
    const uint16_t     errRetryThreshold; // Number of performed retries
    osTimerId_t        timerHandle;       // Safety osTimer used for timer handling
    uint32_t           timerInterval; // Retry interval
    void               (*safetyCallback)(void); // Safety callback
    bool               inError;
}T_OUT_SAFETY_CFG;

typedef struct _T_OUT_CFG
{
    const T_OUT_ID   id;          // Id should reflect position in outsCfg
    const T_OUT_TYPE type;        // Device type - can be BTS500 or BTS72220
    const T_SPOC2_ID spocId;      // If device is BTS72220 this holds sub device id
    const T_SPOC2_CH_ID spocChId; // If device is BTS72220 this holds sub device respecitve channel id
    char             name[OUT_DIAG_NAME_LEN];
    T_OUT_MODE       mode;
    T_OUT_STATE      state;
    T_OUT_STATUS     status;
    T_OUT_ID         batch;       // Optional for OUT_MODE_BATCH (BTS500 only)
    T_OUT_SAFETY_CFG safety;
    uint32_t         currentMA;
    uint32_t         voltageMV;
}T_OUT_CFG;

// Main outputs config
extern T_OUT_CFG outsCfg[];

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id );

void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode);

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE state);

bool OUT_ToggleState(T_OUT_ID id);

bool OUT_Batch(T_OUT_ID id, T_OUT_ID batch);

void OUT_DIAG_AllBts();

void OUT_DIAG_AllSpoc();

uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id);

uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id);

uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id);

T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id);

T_OUT_TYPE OUT_GetType(T_OUT_ID id);

T_OUT_STATE OUT_DIAG_GetState(T_OUT_ID id);

char* OUT_DIAG_GetName(T_OUT_ID id);

#endif