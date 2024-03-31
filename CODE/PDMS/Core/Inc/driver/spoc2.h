#ifndef SPOC2_H_
#define SPOC2_H_

#include "spoc2_definitions.h"
#include "bsp_spoc2.h"
#include <stdint.h>

/// @addtogroup defs Definitions
/// @{

/// @brief The maximum length of a chain of SPOC™ +2 devices. This value can be safely changed by the user to allow for
/// longer chains at the cost of higher memory usage (default: 16).
#define SPOC2_CHAIN_MAX_LEN (16u)

/// @}

typedef uint8_t boolean;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#ifndef NULL
#define NULL ((void*)0)
#endif

/// @addtogroup enums Enumerations
/// @{

/// @brief Success or failure for various operations
typedef enum {
    /// @brief No error
    SPOC2_ERROR_OK = 0,
    /// @brief Error in SPI transmit
    /// @note If this error is returned from a function, the internal shadow registers may be de-synchronized from the
    /// physical devices in the chain. A chain reset should be performed to mitigate this (\ref
    /// SPOC2_Chain_resetAllDevices).
    SPOC2_ERROR_TX_FAILURE,
    /// @brief Error in SPI receive
    /// @note If this error is returned the data in the receive buffer cannot be trusted, as it may contain old data or
    /// partially written data.
    SPOC2_ERROR_RX_FAILURE,
    /// @brief An invalid parameter was passed to the function
    SPOC2_ERROR_BAD_PARAMETER,
    /// @brief The operation is not valid for the selected device
    SPOC2_ERROR_INVALID_OPERATION,
    /// @brief The checksum verification failed (configuration mismatch)
    SPOC2_ERROR_CHECKSUM_FAILURE,
    /// @brief The current sense verification failed (outside the acceptable range)
    SPOC2_ERROR_CURRENT_SENSE_VERIFICATION_FAILURE,
} SPOC2_error_t;

/// @brief Part number of the SPOC+2 device
typedef enum {
    BTS72220_4ESP,
    BTS72220_4ESE,
    BTS72220_4ESA,
    BTS71220_4ESP,
    BTS71220_4ESE,
    BTS71220_4ESA,
    BTS71040_4ESP,
    BTS71040_4ESE,
    BTS71040_4ESA,
    BTS71033_6ESP,
    BTS71033_6ESA,
} SPOC2_deviceIdentifier_t;

/// @brief Input logic modes of SPOC+2 devices
typedef enum {
    /// @brief (default) The output is enabled by the logical OR of the INx pin and the SPI configuration
    SPOC2_INPUT_LOGIC_OR = 0,
    /// @brief The output is enabled by the logical AND of the INx pin and the SPI configuration
    SPOC2_INPUT_LOGIC_AND = 1,
} SPOC2_inputLogicMode_t;

/// @brief Overcurrent thresholds of SPOC+2 device channels
typedef enum {
    /// @brief (default) The high overcurrent threshold as defined by the device datasheet
    SPOC2_OVERCURRENT_THRESHOLD_HIGH = 0,
    /// @brief The low overcurrent threshold as defined by the device datasheet
    SPOC2_OVERCURRENT_THRESHOLD_LOW = 1,
} SPOC2_overCurrentThreshold_t;

/// @enum SPOC2_slewRate_t
/// @brief Slew rate of SPOC+2 device channels
typedef enum {
    /// @brief (default) The normal slew rate as defined by the device datasheet
    SPOC2_SLEW_RATE_NORMAL = 0,
    /// @brief The adjusted slew rate as defined by the device datasheet
    SPOC2_SLEW_RATE_ADJUSTED = 1,
} SPOC2_slewRate_t;

/// @brief Current sense ratios of SPOC+2 device channels
typedef enum {
    /// @brief (default) The high current sense ratio as defined by the device datasheet
    SPOC2_CURRENT_SENSE_RATIO_HIGH = 0,
    /// @brief The low current sense ratio as defined by the device datasheet
    SPOC2_CURRENT_SENSE_RATIO_LOW = 1,
} SPOC2_currentSenseRatio_t;

/// @brief Parallel channel configurations of SPOC+2 devices
typedef enum {
    /// @brief (default) No channels are configured in parallel
    SPOC2_PARALLEL_CHANNELS_NONE = 0,
    /// @brief Channels 0 and 3 are configured in parallel (4-channel devices only)
    SPOC2_PARALLEL_CHANNELS_0_3 = 4,
    /// @brief Channels 1 and 2 are configured in parallel
    SPOC2_PARALLEL_CHANNELS_1_2 = 8,
    /// @brief Channels 0 and 3 are configured in parallel and channels 1 and 2 are configured in parallel (4-channel
    /// devices only)
    SPOC2_PARALLEL_CHANNELS_0_3_AND_1_2 = 4 | 8,
} SPOC2_parallelChannels_t;

/// @brief Restart strategies for SPOC+2 device channels
/// @details For 6-channel devices this behavior can only be configured for channels 1, 2 and 3.
typedef enum {
    /// @brief (default) The channel uses the automatic restart strategy
    /// @details In this mode the device will attempt to automatically restart the channel if it is disabled after
    /// reaching a fault condition (overtemperature or overcurrent) when the restart conditions are met. After a number
    /// of failed restart attempts where fault conditions are reached the channel will enter the latched state and must
    /// be restarted manually, see \ref SPOC2_Config_clearAllRestartCounters. Refer to the device datasheet for more
    /// details
    /// on the fault and restart conditions.
    SPOC2_RESTART_AUTOMATIC = 0,
    /// @brief The channel uses the latching restart strategy
    /// @details In this mode the device will not perform any automatic restarts and the channel will be immediately
    /// latched off when a fault condition is reached. The channel must then be restarted manually, see
    /// \ref SPOC2_Config_clearAllRestartCounters.
    SPOC2_RESTART_LATCH = 1,
} SPOC2_restartBehavior_t;

/// @brief SPOC+2 configuration registers
typedef enum {
    /// @brief The OUTput configuration register (read-write)
    SPOC2_REGISTER_OUT = 0,
    /// @brief The Restart Counter Status register (read-only)
    SPOC2_REGISTER_RCS = 1,
    /// @brief The Slew Rate Control register (read-only)
    SPOC2_REGISTER_SRC = 2,
    /// @brief The OverCuRrent threshold configuration register (read-write)
    SPOC2_REGISTER_OCR = 3,
    /// @brief The Restart Counter Disable register (read-write)
    SPOC2_REGISTER_RCD = 4,
    /// @brief the KILIS (current sense factor) Range Control register (read-write)
    SPOC2_REGISTER_KRC = 5,
    /// @brief The Parallel Channel and Slew rate control register (read-write)
    SPOC2_REGISTER_PCS = 6,
    /// @brief The HardWare ConfiguRation register (read-write)
    SPOC2_REGISTER_HWCR = 7,
    /// @brief The Input status and CheckSum input register (read-write)
    SPOC2_REGISTER_ICS = 8,
    /// @brief The Diagnostic ConfiguRation and swap bit register (read-write)
    SPOC2_REGISTER_DCR = 9,
} SPOC2_register_t;

/// @}

/// @addtogroup structs Data structures
/// @{

/// @brief Configuration of a SPOC+2 device, including shadow registers
typedef struct {
    // Single device identifier in current code logic
    T_SPOC2_ID id;
    /// @brief The product ID of the SPOC+2 device
    SPOC2_deviceIdentifier_t deviceIdentifier;
    /// @brief The logical ADC channel the current sense pin of this device is connected to
    uint32 adcChannel;
    /// @brief The shadow register file
    /// @details **Avoid reading/writing directly from/to this array**, prefer to use the appropriate set/get functions
    /// instead. In case direct reading/writing is necessary, this is possible with e.g.
    /// ```c
    /// SPOC2_deviceConfig_t cfg;
    ///
    /// // read the status of the device outputs from the OUT register (4-channel device)
    /// uint8 outputStatus = cfg.registers[SPOC2_REGISTER_OUT] & SPOC2_4CHANNEL_OUT_OUTn_MASK;
    ///
    /// // set the SWR bit in the DCR register
    /// cfg.registers[SPOC2_REGISTER_DCR] |= SPOC2_DCR_SWR_MASK;
    /// ```
    uint8 registers[10];
} SPOC2_deviceConfig_t;

/// @brief Configuration of a daisy chain of SPOC+2 devices
typedef struct {
    /// @brief The array of connected devices, in order from closest to furthest
    SPOC2_deviceConfig_t devices[SPOC2_CHAIN_MAX_LEN];
    /// @brief The number of connected devices (must be <= SPOC2_CHAIN_MAX_LEN)
    uint32 numDevices;
    /// @brief The logical SPI bus this chain is connected to
    uint32 spiBusId;
    /// @brief The logical chip select number assigned to this chain
    uint8 spiChipSelect;
} SPOC2_chain_t;

typedef struct 
{
    SPOC2_deviceConfig_t devices[SPOC2_NUM_OF_DEVICES];
}SPOC2_config_t;

/// @}

/// @addtogroup userFunctions Required user functions
/// @{

/// @brief User funtion to write a byte buffer to the desired SPI bus using the desired chip select number, starting a
/// SPI transfer
/// @param spiBusId The logical SPI bus to write the data over
/// @param spiChipSelect The logical chip select number to use when writing the SPI data
/// @param txBuffer Pointer to a byte buffer containing the data to be written, must be at least `dataLen` bytes long
/// @param dataLen The number of bytes to be written
/// @return \ref SPOC2_ERROR_OK if no errors occurred, otherwise \ref SPOC2_ERROR_TX_FAILURE
/// @details **This function must be implemented by the driver user.** The device driver uses this function internally
/// to perform SPI communication with device chains. The expected behavior of this function is:
/// - The logical SPI bus provided is mapped to the correct physical SPI communication channel as defined by the user.
/// - The logical chip select number provided is mapped to the correct physical chip select pin as defined by the user.
/// - `dataLen` bytes of data from `txBuffer` are queued to be sent over the SPI interface by e.g. copying to a hardware
/// TX FIFO.
/// - The physical chip select line is held low for the full transmission of `dataLen` bytes. This is requried for
/// correct communication with daisy chains of devices
/// - The function returns with \ref SPOC2_ERROR_OK if all operations were successful or \ref SPOC2_ERROR_TX_FAILURE if
/// a failure occurred. These errors will be propagated by other driver functions.
///
/// This function does not need to block waiting for the SPI transmission to complete, and can return immediately once
/// all the data has been queued. If non-blocking communication is used, the driver internally uses
/// \ref SPOC2_SPI_isTransferComplete to block on transfer completion when necessary.
// Implements: SPOC2_SPI_writeBuffer
extern SPOC2_error_t SPOC2_SPI_writeBuffer(uint32 spiBusId, uint8 spiChipSelect, uint8* txBuffer, uint32 dataLen);

/// @brief User function to read bytes from the desired SPI bus into a byte buffer
/// @param spiBusId The logical SPI bus to read the data from
/// @param rxBuffer Pointer to a byte buffer into which the data will be read, must be at least `dataLen` bytes long
/// @param dataLen The number of bytes to be read
/// @return \ref SPOC2_ERROR_OK if no errors occurred, otherwise \ref SPOC2_ERROR_RX_FAILURE
/// @details **This function must be implemented by the driver user.** The device driver uses this function internally
/// to perform SPI communication with device chains. The expected behavior of this function is:
/// - The logical SPI bus provided is mapped to the correct physical SPI communication channel as defined by the user
/// - `dataLen` bytes are copied from the physical SPI receive buffer into `rxBuffer`
/// - The function returns with \ref SPOC2_ERROR_OK if all operations were successful or \ref SPOC2_ERROR_RX_FAILURE if
/// a failure occurred e.g. less than `dataLen` bytes were available to read. These errors will be propagated by other
/// driver functions.
///
/// On platforms which present an intermediate stage between the SPI communication lines and the available receive
/// buffer, blocking may be required to ensure the correct number of bytes are received.
// Implements: SPOC2_SPI_readRxBuffer
extern SPOC2_error_t SPOC2_SPI_readRxBuffer(uint32 spiBusId, uint8* rxBuffer, uint32 dataLen);

/// @brief User function to empty the SPI receive buffer of unread bytes
/// @param spiBusId The logical SPI bus whose receive buffer should be emptied
/// @details **This function must be implemented by the driver user.** The device driver uses this function internally
/// to perform SPI communication with device chains. The expected behavior of this function is:
/// - The logical SPI bus provided is mapped to the correct physical SPI communication channel as defined by the user
/// - The physical receive buffer of the SPI bus is cleared of any unread data
///
/// This function is used to discard unread data in preparation for a new SPI data transfer.
// Implements: SPOC2_SPI_clearRxBuffer
extern void SPOC2_SPI_clearRxBuffer(uint32 spiBusId);

/// @brief User function to block waiting for the completion of the most recent SPI data transfer (simultaneous
/// transmit/receive)
/// @param spiBusId The logical SPI bus to block on
/// @return `true` if the most recent SPI transfer is complete, `false` otherwise
/// @details **This function must be implemented by the driver user.** The device driver uses this function internally
/// to perform SPI communication with device chains. The expected behavior of this function is:
/// - The logical SPI bus provided is mapped to the correct physical SPI communication channel as defined by the user
/// - The function waits for the previous SPI data transfer to be completed
///
/// This function is used to block waiting for a previous SPI data transfer to allow for asynchronous SPI communication.
/// in the case that \ref SPOC2_SPI_writeBuffer is implemented in a blocking manner, this function can immediately
/// return `true`.
// Implements: SPOC2_SPI_isTransferComplete
extern boolean SPOC2_SPI_isTransferComplete(uint32 spiBusId);

/// @brief User function to read a Millivolt value from an ADC channel
/// @param channel The logical ADC channel to read a value from
/// @return The reading of the ADC channel in Millivolts
/// @details **This function must be implemented by the driver user.** The device driver uses this function internally
/// to read current sense values in Millivolts. The expected behavior of this function is:
/// - The logical ADC channel provided is mapped to the correct hardware ADC channel as defined by the user
/// - A reading is taken from the ADC and converted to Millivolts using the range and resolution of the ADC
/// - The converted value is returned
///
/// The value provided by this function can be used in conjunction with the value of RSENSE defined by the application
/// circuit and the configured KILIS range of the device to calculate the load current. To convert a raw ADC reading
/// into Millivolts, the range and resolution of the ADC must be considered e.g. for a 14-bit ADC with range 0-5V the
/// ADC reading must be multiplied by `5000 / 4096` to obtain a value in mV.
// Implements: SPOC2_ADC_readChannelMilliVolts
extern uint32  SPOC2_ADC_readChannelMilliVolts(uint32 channel);

/// @}

boolean SPOC2_deviceIs6Channel(SPOC2_deviceConfig_t device);
boolean SPOC2_deviceHasExternalDriveCapability(SPOC2_deviceConfig_t device);

SPOC2_error_t SPOC2_Config_enableOutputChannel(SPOC2_deviceConfig_t* cfg, uint8 channel);
SPOC2_error_t SPOC2_Config_disableOutputChannel(SPOC2_deviceConfig_t* cfg, uint8 channel);
SPOC2_error_t SPOC2_Config_enableSleepMode(SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_disableSleepMode(SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_selectCurrentSenseChannel(SPOC2_deviceConfig_t* cfg, uint8 channel);
uint8 SPOC2_Config_getSelectedCurrentSenseChannel(const SPOC2_deviceConfig_t* cfg);
uint8 SPOC2_Config_getChannelCurrentSenseRatios(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_setChannelCurrentSenseRatio(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                       SPOC2_currentSenseRatio_t ratio);
uint8 SPOC2_Config_getChannelOverCurrentThresholds(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_setChannelOverCurrentThreshold(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                          SPOC2_overCurrentThreshold_t overCurrentThreshold);
SPOC2_error_t SPOC2_Config_setInputLogic(SPOC2_deviceConfig_t* cfg, SPOC2_inputLogicMode_t logicMode);
uint8 SPOC2_Config_getChannelSlewRates(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_setSelectedChannelSlewRate(SPOC2_deviceConfig_t* cfg, SPOC2_slewRate_t slewRate);
SPOC2_error_t SPOC2_Config_resetDevice(SPOC2_deviceConfig_t* cfg);
uint8 SPOC2_Config_getChannelRestartBehaviors(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_setChannelRestartBehavior(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                     SPOC2_restartBehavior_t restartBehavior);
SPOC2_error_t SPOC2_Config_enableExternalDriver(SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_disableExternalDriver(SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_selectExternalDriverDiagnostics(SPOC2_deviceConfig_t* cfg);
SPOC2_parallelChannels_t SPOC2_Config_getParallelChannels(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_setParallelChannels(SPOC2_deviceConfig_t* cfg, SPOC2_parallelChannels_t channels);
SPOC2_error_t SPOC2_Config_clearAllRestartCounters(SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_Config_clearSelectedChannelRestartCounter(SPOC2_deviceConfig_t* cfg);

// FIXME This chain functions won't be used     
SPOC2_error_t SPOC2_Chain_writeRegister(SPOC2_chain_t* chain, SPOC2_register_t reg, uint8* txBuffer);
SPOC2_error_t SPOC2_Chain_readRegister(const SPOC2_chain_t* chain, SPOC2_register_t reg, uint8* rxBuffer);
SPOC2_error_t SPOC2_Chain_readInputStates(const SPOC2_chain_t* chain, uint8* rxBuffer);
SPOC2_error_t SPOC2_Chain_readDiagnostics(const SPOC2_chain_t* chain, uint8* stdDiagBuffer, uint8* wrnDiagBuffer,
                                          uint8* errDiagBuffer);

                               
SPOC2_error_t SPOC2_Chain_applyDeviceConfigs(SPOC2_chain_t* chain);
SPOC2_error_t SPOC2_Chain_resetAllDevices(SPOC2_chain_t* chain);
SPOC2_error_t SPOC2_Chain_verifyDeviceConfigs(const SPOC2_chain_t* chain);

uint32 SPOC2_readCurrentSense(const SPOC2_deviceConfig_t* cfg);
SPOC2_error_t SPOC2_verifyCurrentSense(const SPOC2_chain_t* chain, uint8 deviceIndex, uint32 expectedMv);
SPOC2_error_t SPOC2_Chain_init(SPOC2_chain_t* chain);

// FIXME New functions
SPOC2_error_t SPOC2_devinit(SPOC2_deviceConfig_t* dev);
SPOC2_error_t SPOC2_applyDeviceConfig(SPOC2_deviceConfig_t* dev);

// Whole module configuration
void SPOC2_Init();

void SPOC2_SetStdState(T_SPOC2_ID id, T_SPOC2_CH_ID ch, bool state);
void SPOC2_SelectSenseMux(T_SPOC2_ID id, T_SPOC2_CH_ID ch);

#endif // SPOC2_H_