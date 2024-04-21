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

typedef enum
{
    OUT_ERR_NO         = 0x00,  // In case of error nothing happens
    OUT_ERR_LATCH      = 0x01,  // In case of error output is latched till channel reset
    OUT_ERR_OFF        = 0x02,  // In case of error output is turned off till device reset
    OUT_ERR_TIME_LATCH = 0x03   // In case of error output us latched for give amount of time
}T_OUT_ERR_BEHAVIOR;

// After error configuration
typedef struct _T_OUT_SAFETY_AERR_CFG
{
    T_OUT_ERR_BEHAVIOR behavior;
    uint32_t latchTime; // Time for /ref OUT_ERR_TIME_LATCH in ms
}T_OUT_SAFETY_AERR_CFG;
typedef struct _T_OUT_SAFETY_CFG
{
    T_OUT_SAFETY_AERR_CFG aerrCfg;  // Type beahavior if error occurs
    bool               actOnSafety; // This specifies if this channel should be turned of when "safety line" is opened
    bool               useOc;
    uint32_t           ocThreshold;  // Over-current treshold in mili-ampers
    uint16_t           ocTripCounter; // Number of ms to trip of overcurrent
    const uint16_t           ocTripThreshold;  // Number of ms to trip of overcurrent
}T_OUT_SAFETY_CFG;

typedef struct _T_OUT_CFG
{
    const T_OUT_ID   id;          // Id should reflect position in outsCfg
    const T_OUT_TYPE type;        // Device type - can be BTS500 or BTS72220
    const T_SPOC2_ID spocId;      // If device is BTS72220 this holds sub device id
    const T_SPOC2_CH_ID spocChId; // If device is BTS72220 this holds sub device respecitve channel id
    T_OUT_MODE       mode;
    T_OUT_STATE      state;
    uint8_t          status;
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

void OUT_DIAG_All();

uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id);

uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id);

uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id);

T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id);

#endif