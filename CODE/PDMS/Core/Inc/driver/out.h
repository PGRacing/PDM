#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"
#include "spoc2.h"
#include "spoc2_definitions.h"
#include "cmsis_os2.h"

#define OUT_SAFETY_UNLIMITED_RETIRES 0xFFFF
#define OUT_DIAG_NAME_LEN 32

/// @brief Output channel type
typedef enum
{
    OUT_TYPE_BTS500   = 0x00, // Simple high current switch Infineon BTS500 type
    OUT_TYPE_SPOC2    = 0x01, // Complex SPI based switch Infineon BTS72220-4ESA
}T_OUT_TYPE;

/// @brief Output channel when error behavior
typedef enum
{
    OUT_ERR_BEH_NO         = 0x00,  // In case of error nothing happens
    OUT_ERR_BEH_LATCH      = 0x01,  // In case of error output is latched till channel reset
    OUT_ERR_BEH_TIME_LATCH = 0x03,  // In case of error output is latched for give amount of time
    OUT_ERR_BEH_TRY_RETRY  = 0x04,  // In case of error output there are few retries before latch
}T_OUT_ERR_BEHAVIOR;

/// @brief Output channel software over current status
typedef enum
{
    OUT_SAFETY_SOC_PRE_INRUSH       = 0x00,
    OUT_SAFETY_SOC_INRUSH_WINDOW    = 0x01,
    OUT_SAFETY_SOC_NORMAL_OPERATION = 0x02,
    OUT_SAFETY_SOC_TRIGGERED        = 0x03,
}
T_OUT_SAFETY_SOC_STATUS;

/// @brief Output chhannel after error configuration
typedef struct _T_OUT_SAFETY_AERR_CFG
{
    T_OUT_ERR_BEHAVIOR behavior; // After error routine selection
    uint32_t latchTime;          // Time for /ref OUT_ERR_BEH_TIME_LATCH in ms
}T_OUT_SAFETY_AERR_CFG;

/// @brief Output channel software overcurrent configuration
typedef struct _T_OUT_SAFETY_SOC_CFG
{
    bool               useSoc;                  // Should software over current function be used
    const uint32_t     nominalThreshold;         // Allowed maximal current during nominal operation [mA]
    // Inrush current capability
    bool               allowInrush;             // Should inrush current be allowed during SOC operation
    const uint32_t     inrushWindowFromStart;   // Time window after channel turn-on in which inrush current is allowed
    const uint32_t     inrushTreshold;          // Allowed maximal current during inrush operation [mA]
    const uint32_t     inrushTimeThreshold;      // For how long inrush current can be present after first peak [ms] 
}T_OUT_SAFETY_SOC_CFG;

/// @brief Output channel safety configuration struct
typedef struct _T_OUT_SAFETY_CFG
{
    T_OUT_SAFETY_AERR_CFG aerrCfg;        // Type beahavior if error occurs
    bool               actOnSafety;       // This specifies if this channel should be turned of when "safety line" is opened
    bool               useSoc;            // Use software over current function?
    uint32_t           socThreshold;      // Over-current treshold in mili-ampers
    const uint16_t     socTripThreshold;  // Number of ms to trip of overcurrent
    const uint16_t     errRetryThreshold; // Number of performed retries
    uint32_t           timerInterval;     // Retry interval
    void               (*safetyCallback)(void); // Safety callback
    T_OUT_SAFETY_SOC_CFG socCfg;
}T_OUT_SAFETY_CFG; 


/// @brief Output channel configuration struct
typedef struct _T_OUT_CFG
{
    const T_OUT_ID   id;                  // Id should reflect position in outsCfg
    const T_OUT_TYPE type;                // Device type - can be BTS500 or BTS72220
    T_OUT_MODE       mode;                // Output mode (TYP / PWM / BATCH / ...)
    const T_SPOC2_ID spocId;              // If device is BTS72220 this holds sub device id
    const T_SPOC2_CH_ID spocChId;         // If device is BTS72220 this holds sub device respecitve channel id
    char             name[OUT_DIAG_NAME_LEN]; // Output channel pretty name
    T_OUT_ID         batch;               // Optional for OUT_MODE_BATCH (BTS500 only)
    T_OUT_SAFETY_CFG safety;              // Safety configuration
}T_OUT_CFG;

/// @brief Output channel software overcurrent status register
typedef struct _T_OUT_SAFETY_SOC_REG
{
    T_OUT_SAFETY_SOC_STATUS status;            // Software over-current system status
    uint32_t                currentThreshold;   // Currently applicable current treshold - can be changed dynamically (Ith) [mA]
    uint32_t                tripCounter;       // Counter incremented when current is over treshold
    uint32_t                timeInPreInrush;   // Couter used to measure time from channel start
}
T_OUT_SAFETY_SOC_REG;

/// @brief Output channel safety state register
typedef struct _T_OUT_SAFETY_REG
{
    uint16_t                  ocTripCounter;    // Counter of ms to trip of overcurrent
    uint16_t                  errRetryCounter;  // Counter of performed retries
    osTimerId_t               timerHandle;      // Safety osTimer used for timer handling
    bool                      inError;          // Is output channel in error mode?
    T_OUT_SAFETY_SOC_REG      socReg;           // software over current status register
}T_OUT_SAFETY_REG;

/// @brief Output channel state register
typedef struct _T_OUT_REG
{
    T_OUT_STATE      state;       // Output state (ON / OFF)
    T_OUT_STATUS     status;      // Output status 
    T_OUT_SAFETY_REG safety;      // Safety state register
    // Acquired from board
    uint32_t         currentMA;   // Here for ease of debug and code simplification
    uint32_t         voltageMV; 
    // PWM mode
    uint_fast8_t     pwmDuty;      // PWM duty
    uint_fast8_t     prevPwmDuty; // Previous pwm duty (before last update)
}T_OUT_REG;

// Main outputs config
extern T_OUT_CFG outsCfg[];

// Main outputs status
extern T_OUT_REG outsReg[];

/// @brief Change mode of selected output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @param targetMode [UNUSED/STD/PWM/BATCH]
/// @note Remember this function will reinit even if targetMode == currentMode
void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode);

/// @brief Set state of selected output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @param reqState Requested output channel state 
/// @return FALSE if error occured, TRUE if ok
bool OUT_SetState(T_OUT_ID id, T_OUT_STATE reqState);

/// @brief Toggle [on/off] state of selected output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @return FALSE if error occured, TRUE if ok
bool OUT_ToggleState(T_OUT_ID id);

/// @brief Set PWM duty in percentage
/// @param id Output channel id [1..16] T_OUT_ID
/// @param duty PWM signal duty 0 - 100%
/// @return FALSE if error occured, TRUE if ok
bool OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty);

/// @brief Batch output channels to work simultaneously
/// @param id Output channel id [1..16] T_OUT_ID
/// @param batchId Batch output channel id [1..16] T_OUT_ID
/// @return FALSE if error occured, TRUE if ok
/// @note For BTS500 you can batch between 1-4 and 5-8
bool OUT_Batch(T_OUT_ID id, T_OUT_ID batchId);

/// @brief Perform all needed processing for BTS type output channels
/// @note  Fast control loop body
void OUT_DIAG_AllBts(void);

/// @brief Perform all needed processing for SPOC2 type output channels
/// @note  Fast control loop body
void OUT_DIAG_AllSpoc(void);

/// @brief Get output channel current value in mA range
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical current value [mA]
uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id);

/// @brief Get output channel voltage value in mV range
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical voltage value [mV]
uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id);

/// @brief Get output channel current value in pA range
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical current value [pA]
uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id);

/// @brief Get output channel status value
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output channel status
T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id);

/// @brief Get output channel status value
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output channel state
T_OUT_STATE OUT_DIAG_GetState(T_OUT_ID id);

/// @brief Get output channel type
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output channel type
T_OUT_TYPE OUT_GetType(T_OUT_ID id);

/// @brief Get output channel name
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output channel name
char* OUT_GetName(T_OUT_ID id);

/// @brief Get output channel mode
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Output channel mode
T_OUT_MODE OUT_GetMode(T_OUT_ID id);

/// @brief Perform output handling of all SPOC type elements
void OUT_DIAG_AllSpoc(void);




#endif