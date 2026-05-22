#ifndef __OUT_H_
#define __OUT_H_

#include "typedefs.h"
#include "bsp_out.h"
#include "spoc2.h"
#include "spoc2_definitions.h"
#include "cmsis_os2.h"

#define OUT_SAFETY_UNLIMITED_RETIRES 0xFFFF
#define OUT_DIAG_NAME_LEN 32

#pragma pack(push, 1)

/// @brief Output channel type
typedef enum
{
    OUT_TYPE_BTS500   = 0x00, // Simple high current switch Infineon BTS500 type
    OUT_TYPE_SPOC2    = 0x01, // Complex SPI based switch Infineon BTS72220-4ESA
}T_OUT_TYPE;

/// @brief Output channel status
/// @note Sorted by priority
typedef enum
{
    OUT_STATUS_OK            = 0,
    OUT_STATUS_OPEN_LOAD     = 1,
    OUT_STATUS_SAFETY_OPEN   = 8,

    OUT_STATUS_PRIORITY_DIV  = 9,
    // Software protection position
    OUT_STATUS_SOC_FAULT     = 10,
    OUT_STATUS_I2T_FAULT     = 11,

    // Hardware protection (always higher priority)
    OUT_STATUS_SHORT_TO_VSS  = 20,
    OUT_STATUS_CONTROL_FAIL  = 21,
    OUT_STATUS_HARD_FAULT    = 22,
}T_OUT_STATUS;

/// @brief Output channel when error behavior
typedef enum
{
    OUT_ERR_BEH_NO         = 0x00,  // In case of error nothing happens
    OUT_ERR_BEH_LATCH      = 0x01,  // In case of error output is latched till channel reset
    OUT_ERR_BEH_TIME_LATCH = 0x03,  // In case of error output is latched for give amount of time
    OUT_ERR_BEH_RETRY  = 0x04,  // In case of error output there are few retries before latch
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

/// TODO Verify which parameters should be declared as const
/// @brief Output chhannel after error configuration
typedef struct _T_OUT_SAFETY_AERR_CFG
{
    T_OUT_ERR_BEHAVIOR behavior; // After error routine selection
    uint32_t latchTime;          // Time for /ref OUT_ERR_BEH_TIME_LATCH in ms /UNUSED/
}T_OUT_SAFETY_AERR_CFG;

/// @brief Output channel software overcurrent configuration
typedef struct _T_OUT_SAFETY_SOC_CFG
{
    bool               useSoc;                  // Should software over current function be used
    const uint32_t     nominalThreshold;         // Allowed maximal current during nominal operation [mA]
    // Inrush current capability
    bool               allowInrush;             // Should inrush current be allowed during SOC operation
    const uint32_t     inrushWindowFromStart;   // Time window after channel turn-on in which inrush current is allowed
    const uint32_t     inrushThreshold;          // Allowed maximal current during inrush operation [mA]
    const uint32_t     inrushTimeThreshold;      // For how long inrush current can be present after first peak [ms] 
}T_OUT_SAFETY_SOC_CFG;

/// @brief Output channel I2t configuration
typedef struct _T_OUT_SAFETY_I2T_CFG
{
    bool               useI2t;                  // Should software I2t function be used
    const uint32_t     nominalCurrent;          // I2t nominal current [mA/10] eg. 12A = 1200 ; 3.3A = 330 0.1A = 10 (max value 40A)
    const uint32_t     nominalCurrentSq;        // I2t nominal current squared (fill in via nominal current)
    const uint32_t     timeThreshold;           // I2t time value
    const uint32_t     i2tThreshold;            // I2t threshold (nominalCurrentSq * timeThreshold)
}T_OUT_SAFETY_I2T_CFG;

/// @brief Output channel safety configuration struct
typedef struct _T_OUT_SAFETY_CFG
{
    T_OUT_SAFETY_AERR_CFG afterErrorCfg;          // Configuration of after error / fault behavior
    bool                  actOnSafety;            // This specifies if this channel should be turned of when "safety line" is opened
    const uint16_t        errRetryThreshold;      // Number of retries to be performed
    uint32_t              retryTimerInterval;     // Retry interval
    T_OUT_SAFETY_SOC_CFG  socCfg;                 // Software over current configuration
    T_OUT_SAFETY_I2T_CFG  i2tCfg;                 // I2t configuration
}T_OUT_SAFETY_CFG; 

/// @brief Output channel configuration struct
typedef struct _T_OUT_CFG
{
    const T_OUT_ID   id;                      // Id should reflect position in outsCfg
    const T_OUT_TYPE type;                    // Device type - can be BTS500 or BTS72220
    T_OUT_MODE       mode;                    // Output mode (TYP / PWM / BATCH / ...)
    const T_SPOC2_ID spocId;                  // If device is BTS72220 this holds sub device id
    const T_SPOC2_CH_ID spocChId;             // If device is BTS72220 this holds sub device respecitve channel id
    char             name[OUT_DIAG_NAME_LEN]; // Output channel pretty name
    T_OUT_ID         batch;                   // Optional for OUT_MODE_BATCH (BTS500 only)
    T_OUT_SAFETY_CFG safety;                  // Safety configuration
}T_OUT_CFG;

#pragma pack(pop)

/// @brief Output channel software overcurrent status register
typedef struct _T_OUT_SAFETY_SOC_REG
{
    T_OUT_SAFETY_SOC_STATUS status;            // Software over-current system status
    uint32_t                currentThreshold;  // Currently applicable current threshold - can be changed dynamically (Ith) [mA]
    uint32_t                inrushTripCounter; // Counter incremented when in OUT_SAFETY_SOC_INRUSH_WINDOW
    uint32_t                timeInPreInrush;   // Couter used to measure time from channel start
    uint32_t                thresholdExceedCounter; // How many times current threshold was exceeded
}
T_OUT_SAFETY_SOC_REG;


/// @brief Output channel I2t status register
typedef struct _T_OUT_SAFETY_I2T_REG
{
    int_fast64_t i2tSum;
}
T_OUT_SAFETY_I2T_REG;

/// @brief Output channel safety state register
typedef struct _T_OUT_SAFETY_REG
{
    bool                      inRetrySequence;   // Is output channel in error mode?
    uint_fast16_t             errRetryCounter;   // Counter of performed retries
    uint32_t                  retryTimerCounter; // Timer counting between retries
    T_OUT_SAFETY_SOC_REG      socReg;            // Software over current status register
    T_OUT_SAFETY_I2T_REG      i2tReg;            // I2t status register
}T_OUT_SAFETY_REG;

/// @brief Output channel state register
typedef struct _T_OUT_REG
{
    T_OUT_STATE      state;       // Output state (ON / OFF)
    T_OUT_STATUS     status;      // Output status 
    T_OUT_SAFETY_REG safety;      // Safety status register
    // Acquired from board
    uint32_t         currentMA;   // Here for ease of debug and code simplification
    uint32_t         voltageMV; 
    uint32_t         emaCurrentMA; // Exponential moving average of current for smoother readings
    uint32_t         emaVoltageMV; // Exponential moving average of voltage for smoother readings   
    // PWM mode
    uint_fast8_t     pwmDuty;      // PWM duty
    uint_fast8_t     prevPwmDuty; // Previous pwm duty (before last update)
}T_OUT_REG;

// Main outputs config
extern T_OUT_CFG outsCfg[OUT_ID_MAX];

// Main outputs status
extern T_OUT_REG outsReg[OUT_ID_MAX];

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

/// @brief Get output channel voltage value in mV range from EMA
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical voltage value [mV]
uint32_t OUT_DIAG_GetEmaVoltage(T_OUT_ID id);

/// @brief Get output channel current value in pA range
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical current value [pA]
uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id);

/// @brief Get output channel current value in pA range from EMA
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Electrical current value [pA]
uint16_t OUT_DIAG_GetEmaCurrent_pA(T_OUT_ID id);

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

/// @brief Reset all output channel registers to default values
void OUT_ResetRegistersAll(void);



#endif