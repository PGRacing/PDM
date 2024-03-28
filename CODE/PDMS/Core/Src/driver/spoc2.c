#include "spoc2.h"

/// @addtogroup driverFunctions Driver functions
/// @{

/// @brief Indicates whether a device is a 6-channel device
/// @param device The configuration structure of the relevant device
/// @return `true` if `device` is a 6-channel device, else `false`
// Implements: SPOC2_deviceIs6Channel
boolean SPOC2_deviceIs6Channel(SPOC2_deviceConfig_t device) {

    return (device.deviceIdentifier == BTS71033_6ESP) || (device.deviceIdentifier == BTS71033_6ESA);
}

/// @brief Indicates whether a device can drive an external smart power switch
/// @param device The configuration structure of the relevant device
/// @return `true` if the device can drive an external smart power switch, else `false`
// Implements: SPOC2_deviceHasExternalDriveCapability
boolean SPOC2_deviceHasExternalDriveCapability(SPOC2_deviceConfig_t device) {
    return (device.deviceIdentifier == BTS71040_4ESE) || (device.deviceIdentifier == BTS71220_4ESE) ||
           (device.deviceIdentifier == BTS72220_4ESE);
}

// Implements: S2DD-VD1_IPVSR-20
static SPOC2_error_t SPOC2_SPI_transferBlocking(uint32 spiBusId, uint8 spiChipSelect, uint8* txBuffer, uint8* rxBuffer,
                                                uint32 dataLen) {
    SPOC2_error_t result = SPOC2_ERROR_OK;

    // clear the RX buffer of unread data
    SPOC2_SPI_clearRxBuffer(spiBusId);

    result = SPOC2_SPI_writeBuffer(spiBusId, spiChipSelect, txBuffer, dataLen);

    // block waiting for this transfer to complete
    while (!SPOC2_SPI_isTransferComplete(spiBusId)) {}

    if ((result == SPOC2_ERROR_OK) && (rxBuffer != NULL)) {
        result = SPOC2_SPI_readRxBuffer(spiBusId, rxBuffer, dataLen);
    } else {
        // do nothing
    }

    return result;
}

// FIXME (Add notes) New transfer function based on BSP
static SPOC2_error_t SPOC2_SPI_transferBlocking2(T_SPOC2_ID id, uint8* txBuffer, uint8* rxBuffer, uint32 dataLen)
{
    SPOC2_error_t result = SPOC2_ERROR_OK;

    bool status = BSP_SPOC2_TransferBlocking(id, txBuffer, rxBuffer, dataLen);

    result = (status) ? SPOC2_ERROR_OK : SPOC2_ERROR_INVALID_OPERATION;

    return result;

}

/// @brief Enables the desired channel in the specified device configuration
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to enable, should be 0-3 for 4-channel devices and 0-5 for 6-channel devices
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel is out of range
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_enableOutputChannel
// Implements: S2DD-VD1_IPVSR-10
SPOC2_error_t SPOC2_Config_enableOutputChannel(SPOC2_deviceConfig_t* cfg, uint8 channel) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    if (SPOC2_deviceIs6Channel(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                     // pointer must be verified by the API user"

        && (channel < 6)) {
        cfg->registers[SPOC2_REGISTER_OUT] |= (1u << channel);
    } else if (channel < 4) {
        cfg->registers[SPOC2_REGISTER_OUT] |= (1u << channel);
    } else {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    return result;
}

/// @brief Disables the desired channel in the specified device configuration
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to disable, should be 0-3 for 4-channel devices and 0-5 for 6-channel devices
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel is out of range
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_disableOutputChannel
// Implements: S2DD-VD1_IPVSR-11
SPOC2_error_t SPOC2_Config_disableOutputChannel(SPOC2_deviceConfig_t* cfg, uint8 channel) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    if ((SPOC2_deviceIs6Channel(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                      // pointer must be verified by the API user"

         && (channel < 6u)) ||
        (channel < 4u)) {
        cfg->registers[SPOC2_REGISTER_OUT] &= ~(1u << channel);
    } else {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    return result;
}

/// @brief Enables sleep mode in the the specified device configuration
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_enableSleepMode
// Implements: S2DD-VD1_IPVSR-8
SPOC2_error_t SPOC2_Config_enableSleepMode(SPOC2_deviceConfig_t* cfg) {
    // setting DCR.MUX to 0b111 enables sleep mode
    cfg->registers[SPOC2_REGISTER_DCR] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                       // pointer must be verified by the API user"

        |= SPOC2_DCR_MUX_MASK;
    return SPOC2_ERROR_OK;
}

/// @brief Disables sleep mode in the the specified device configuration
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_disableSleepMode
// Implements: S2DD-VD1_IPVSR-9
SPOC2_error_t SPOC2_Config_disableSleepMode(SPOC2_deviceConfig_t* cfg) {
    // set DCR.MUX to 0b111
    cfg->registers[SPOC2_REGISTER_DCR] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                       // pointer must be verified by the API user"

        &= ~SPOC2_DCR_MUX_MASK;
    return SPOC2_ERROR_OK;
}

/// @brief Select the channel of the specified device that the current sense function will measure
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to measure with the current sense function, should be 0-3 for 4-channel devices and 0-5
/// for 6-channel devices
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel is out of range
/// @details **Note:** due to the register structure of the SPOC™ +2 family, applying this configuration change will force
/// a device out of sleep mode. The configuration change made by this function will not be applied until the next call
/// to \ref SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_selectCurrentSenseChannel
// Implements: S2DD-VD1_IPVSR-7
SPOC2_error_t SPOC2_Config_selectCurrentSenseChannel(SPOC2_deviceConfig_t* cfg, uint8 channel) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    boolean externalDrive =
        SPOC2_deviceHasExternalDriveCapability(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                      // pointer must be verified by the API user"
    if ((SPOC2_deviceIs6Channel(*cfg) && (channel < 6u)) ||
        // allow selecting channel 4 on external-driver-capable devices (this is used internally)
        (externalDrive && channel < 5) || (channel < 4u)) {

        cfg->registers[SPOC2_REGISTER_DCR] =
            (cfg->registers[SPOC2_REGISTER_DCR] & ~SPOC2_DCR_MUX_MASK) | (channel & SPOC2_DCR_MUX_MASK);
    } else {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    return result;
}

/// @brief Get the channel of the specified device currently selected for current sense measurement
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return The index of the currently selected current sense channel
/// @details **Note**: a returned value of 7 indicates that the device is in sleep mode. For 4-channel devices with
/// external driver capability, a returned value of 4 indicates that the external driver diagnostics are routed to the
/// current sense pin.
// Implements: SPOC2_Config_getSelectedCurrentSenseChannel
uint8 SPOC2_Config_getSelectedCurrentSenseChannel(const SPOC2_deviceConfig_t* cfg) {
    return cfg->registers[SPOC2_REGISTER_DCR] & // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                // pointer must be verified by the API user"
           SPOC2_DCR_MUX_MASK;
}

/// @brief Get the currently configured channel current sense ratios for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return A mask of the configured current sense ratios
/// @details The bits in the returned mask represent the current sense ratio for their respective channels e.g. a `1`
/// bit at position 3 indicates that channel 3 is configured with the low current sense ratio:
/// ```c
/// uint8 ratios = SPOC2_Config_getChannelCurrentSenseRatios(&cfg);
///
/// // extract the configuration for channel 3
/// boolean channel3LowRatio = (ratios & (1 << 3)) != 0;
/// ```
// Implements: SPOC2_Config_getChannelCurrentSenseRatios
uint8 SPOC2_Config_getChannelCurrentSenseRatios(const SPOC2_deviceConfig_t* cfg) {
    // the 4 channel mask also fits the 6 channel register
    return cfg->registers[SPOC2_REGISTER_KRC] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                              // pointer must be verified by the API user"

           & SPOC2_4CHANNEL_KRC_KRCn_MASK;
}

/// @brief Set the current sense ratio for the specified channel and device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to configure
/// @param ratio The desired current sense ratio for the channel
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel in not configurable for the
/// device
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_setChannelCurrentSenseRatio
// Implements: S2DD-VD1_IPVSR-6
SPOC2_error_t SPOC2_Config_setChannelCurrentSenseRatio(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                       SPOC2_currentSenseRatio_t ratio) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    boolean is6Channel;
    is6Channel = SPOC2_deviceIs6Channel(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                               // pointer must be verified by the API user"

    if ((is6Channel && ((channel < 1u) || (channel > 3u))) || (!is6Channel && (channel > 3u))) {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    if (result == SPOC2_ERROR_OK) {
        if (ratio == SPOC2_CURRENT_SENSE_RATIO_LOW) {
            cfg->registers[SPOC2_REGISTER_KRC] |= 1u << channel;
        } else if (ratio == SPOC2_CURRENT_SENSE_RATIO_HIGH) {
            cfg->registers[SPOC2_REGISTER_KRC] &= ~(1u << channel);
        } else {
            result = SPOC2_ERROR_BAD_PARAMETER;
        }
    } else {
        // do nothing
    }

    return result;
}

/// @brief Get the currently configured channel overcurrent thresholds for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return A mask of the configured overcurrent thresholds
/// @details The bits in the returned mask represent the overcurrent thresholds for their respective channels e.g. a `1`
/// bit at position 3 indicates that channel 3 is configured with the low overcurrent threshold:
/// ```c
/// uint8 thresholds = SPOC2_Config_getChannelOverCurrentThresholds(&cfg);
///
/// // extract the configuration for channel 3
/// boolean channel3LowThreshold = (thresholds & (1 << 3)) != 0;
/// ```
// Implements: SPOC2_Config_getChannelOverCurrentThresholds
uint8 SPOC2_Config_getChannelOverCurrentThresholds(const SPOC2_deviceConfig_t* cfg) {
    // the 4 channel mask also fits the 6 channel register
    return cfg->registers[SPOC2_REGISTER_OCR] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                              // pointer must be verified by the API user"

           & SPOC2_4CHANNEL_OCR_OCTn_MASK;
}

/// SPOC2_overCurrentThreshold_t overCurrentThreshold)
/// @brief Set the overcurrent threshold for the specified channel and device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to configure
/// @param ratio The desired overcurrent threshold for the channel
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel in not configurable for the
/// device
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_setChannelOverCurrentThreshold
// Implements: S2DD-VD1_IPVSR-15
SPOC2_error_t SPOC2_Config_setChannelOverCurrentThreshold(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                          SPOC2_overCurrentThreshold_t overCurrentThreshold) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    if (SPOC2_deviceIs6Channel(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                     // pointer must be verified by the API user"

        && (channel < 4u) && (channel > 0u)) {
        if (overCurrentThreshold == SPOC2_OVERCURRENT_THRESHOLD_LOW) {
            cfg->registers[SPOC2_REGISTER_OCR] |= 1u << channel;
        } else {
            cfg->registers[SPOC2_REGISTER_OCR] &= ~(1u << channel);
        }
    } else if ((!SPOC2_deviceIs6Channel(*cfg)) && (channel < 4u)) {
        if (overCurrentThreshold == SPOC2_OVERCURRENT_THRESHOLD_LOW) {
            cfg->registers[SPOC2_REGISTER_OCR] |= 1u << channel;
        } else {
            cfg->registers[SPOC2_REGISTER_OCR] &= ~(1u << channel);
        }
    } else {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    return result;
}

/// @brief Set the input logic mode for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param logicMode The desired input logic mode
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_setInputLogic
// Implements: S2DD-VD1_IPVSR-17
SPOC2_error_t SPOC2_Config_setInputLogic(SPOC2_deviceConfig_t* cfg, SPOC2_inputLogicMode_t logicMode) {
    if (logicMode == SPOC2_INPUT_LOGIC_AND) {
        cfg->registers[SPOC2_REGISTER_HWCR] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                            // pointer must be verified by the API user"

            |= SPOC2_HWCR_COL_MASK;
    } else {
        cfg->registers[SPOC2_REGISTER_HWCR] // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                            // pointer must be verified by the API user"

            &= ~SPOC2_HWCR_COL_MASK;
    }
    return SPOC2_ERROR_OK;
}

/// @brief Get the currently configured channel slew rates for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return A mask of the configured slew rates
/// @details The bits in the returned mask represent the slew rates for their respective channels e.g. a `1`
/// bit at position 3 indicates that channel 3 is configured with the adjusted slew rate:
/// ```c
/// uint8 rates = SPOC2_Config_getChannelSlewRates(&cfg);
///
/// // extract the configuration for channel 3
/// boolean channel3AdjustedRate = (rates & (1 << 3)) != 0;
/// ```
// Implements: SPOC2_Config_getChannelSlewRates
uint8 SPOC2_Config_getChannelSlewRates(const SPOC2_deviceConfig_t* cfg) {
    return cfg->registers[SPOC2_REGISTER_SRC] & // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                // pointer must be verified by the API user"
           SPOC2_6CHANNEL_SRC_SRCn_MASK;
}

/// @brief Set the slew rate for the currently selected current sense channel in the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param logicMode The desired slew rate
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_INVALID_OPERATION if no channel is currently selected by
/// the device multiplexer
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs. If a slew rate change and a channel selection change are both queued, the slew rate
/// will be applied to the newly selected channel
// Implements: SPOC2_Config_setSelectedChannelSlewRate
// Implements: S2DD-VD1_IPVSR-12
SPOC2_error_t SPOC2_Config_setSelectedChannelSlewRate(SPOC2_deviceConfig_t* cfg, SPOC2_slewRate_t slewRate) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    boolean is6Channel;
    is6Channel = SPOC2_deviceIs6Channel(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                               // pointer must be verified by the API user"

    if ((is6Channel && (((cfg->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK) >= 6u))) ||
        (!is6Channel && ((cfg->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK) >= 4u))) {
        // no channel is selected by the multiplexer
        result = SPOC2_ERROR_INVALID_OPERATION;
    }

    if (result == SPOC2_ERROR_OK) {

        if (slewRate == SPOC2_SLEW_RATE_ADJUSTED) {
            cfg->registers[SPOC2_REGISTER_PCS] |= SPOC2_PCS_SRCS_MASK;
            cfg->registers[SPOC2_REGISTER_SRC] |= 1u << (cfg->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK);
        } else if (slewRate == SPOC2_SLEW_RATE_NORMAL) {
            cfg->registers[SPOC2_REGISTER_PCS] &= ~SPOC2_PCS_SRCS_MASK;
            cfg->registers[SPOC2_REGISTER_SRC] &= ~(1u << (cfg->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK));
        } else {
            result = SPOC2_ERROR_BAD_PARAMETER;
        }
    } else {
        // do nothing
    }

    return result;
}

/// @brief Reset a device configuration to default values
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_resetDevice
SPOC2_error_t SPOC2_Config_resetDevice(SPOC2_deviceConfig_t* cfg) {
    // reset registers to their default values
    cfg->registers[SPOC2_REGISTER_OUT] = 0x80;
    cfg->registers[SPOC2_REGISTER_RCS] = 0x80;
    cfg->registers[SPOC2_REGISTER_SRC] = 0x90;
    cfg->registers[SPOC2_REGISTER_OCR] = 0xC0;
    cfg->registers[SPOC2_REGISTER_RCD] = 0xC0;
    cfg->registers[SPOC2_REGISTER_KRC] = 0xD0;
    cfg->registers[SPOC2_REGISTER_PCS] = 0xD0;
    cfg->registers[SPOC2_REGISTER_HWCR] = 0xE0;
    cfg->registers[SPOC2_REGISTER_ICS] = 0xE0;
    cfg->registers[SPOC2_REGISTER_DCR] = 0xF7;

    // one difference for 6 channel device
    if (SPOC2_deviceIs6Channel(*cfg)) {
        cfg->registers[SPOC2_REGISTER_SRC] = 0x80;
    }

    return SPOC2_ERROR_OK;
}

/// @brief Get the currently configured channel restart behaviors for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return A mask of the configured restart behaviors
/// @details The bits in the returned mask represent the restart behaviors for their respective channels e.g. a `1`
/// bit at position 3 indicates that channel 3 is configured with the latching restart behavior:
/// ```c
/// uint8 restarts = SPOC2_Config_getChannelRestartBehaviors(&cfg);
///
/// // extract the configuration for channel 3
/// boolean channel3LatchingRestart = (restarts & (1 << 3)) != 0;
/// ```
// Implements: SPOC2_Config_getChannelRestartBehaviors
uint8 SPOC2_Config_getChannelRestartBehaviors(const SPOC2_deviceConfig_t* cfg) {
    uint8 mask = SPOC2_deviceIs6Channel(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of
                                              // the pointer must be verified by the API user"
                     ? SPOC2_6CHANNEL_RCD_RCDn_MASK
                     : SPOC2_4CHANNEL_RCD_RCDn_MASK;
    return cfg->registers[SPOC2_REGISTER_RCD] & mask;
}

/// @brief Set the restart behavior for the specified channel and device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channel The channel to configure
/// @param ratio The desired restart behavior for the channel
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the channel in not configurable for the
/// device
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_setChannelRestartBehavior
// Implements: S2DD-VD1_IPVSR-14
SPOC2_error_t SPOC2_Config_setChannelRestartBehavior(SPOC2_deviceConfig_t* cfg, uint8 channel,
                                                     SPOC2_restartBehavior_t restartBehavior) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    boolean is6Channel = SPOC2_deviceIs6Channel(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                       // pointer must be verified by the API user"
    if (is6Channel && (channel > 0u) && (channel < 4u)) {
        // 6 channel devices
        if (restartBehavior == SPOC2_RESTART_LATCH) {
            cfg->registers[SPOC2_REGISTER_RCD] |= (1u << channel);
        } else {
            cfg->registers[SPOC2_REGISTER_RCD] &= ~(1u << channel);
        }
    } else if (!is6Channel && (channel < 4u)) {
        // 4 channel devices
        if (restartBehavior == SPOC2_RESTART_LATCH) {
            cfg->registers[SPOC2_REGISTER_RCD] |= (1u << channel);
        } else {
            cfg->registers[SPOC2_REGISTER_RCD] &= ~(1u << channel);
        }
    } else {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }
    return result;
}

/// @brief Enable the external smart power switch on supported devices
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK if the device has external drive capability, else \ref SPOC2_ERROR_INVALID_OPERATION
// Implements: SPOC2_Config_enableExternalDriver
// Implements: S2DD-VD1_IPVSR-18
SPOC2_error_t SPOC2_Config_enableExternalDriver(SPOC2_deviceConfig_t* cfg) {
    SPOC2_error_t result;
    if (SPOC2_deviceHasExternalDriveCapability(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                     // pointer must be verified by the API user"

    ) {
        cfg->registers[SPOC2_REGISTER_OUT] |= 1u << 4u;
        result = SPOC2_ERROR_OK;
    } else {
        result = SPOC2_ERROR_INVALID_OPERATION;
    }
    return result;
}

/// @brief Disable the external smart power switch on supported devices
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK if the device has external drive capability, else \ref SPOC2_ERROR_INVALID_OPERATION
// Implements: SPOC2_Config_disableExternalDriver
// Implements: S2DD-VD1_IPVSR-19
SPOC2_error_t SPOC2_Config_disableExternalDriver(SPOC2_deviceConfig_t* cfg) {
    SPOC2_error_t result;

    if (SPOC2_deviceHasExternalDriveCapability(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                     // pointer must be verified by the API user"
    ) {
        cfg->registers[SPOC2_REGISTER_OUT] &= ~(1u << 4u);
        result = SPOC2_ERROR_OK;
    } else {
        result = SPOC2_ERROR_INVALID_OPERATION;
    }

    return result;
}

/// @brief Selects the external smart power switch to be measured with the device current sense pin
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK if the device has external drive capability, else SPOC2_ERROR_INVALID_OPERATION
// Implements: SPOC2_Config_selectExternalDriverDiagnostics
SPOC2_error_t SPOC2_Config_selectExternalDriverDiagnostics(SPOC2_deviceConfig_t* cfg) {
    SPOC2_error_t result;
    if (SPOC2_deviceHasExternalDriveCapability(*cfg) // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                     // pointer must be verified by the API user"

    ) {
        (void)SPOC2_Config_selectCurrentSenseChannel(cfg, 4u);
        result = SPOC2_ERROR_OK;
    } else {
        result = SPOC2_ERROR_INVALID_OPERATION;
    }

    return result;
}

/// @brief Gets the current parallel channel configuration for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return The current parallel channel configuration for the specified device
// Implements: SPOC2_Config_getParallelChannels
SPOC2_parallelChannels_t SPOC2_Config_getParallelChannels(const SPOC2_deviceConfig_t* cfg) {
    uint8 result;
    SPOC2_parallelChannels_t enumResult;
    boolean is6Channel = SPOC2_deviceIs6Channel(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                       // pointer must be verified by the API user"

    if (is6Channel) {
        result = cfg->registers[SPOC2_REGISTER_PCS] & SPOC2_6CHANNEL_PCS_PCCn_MASK;
    } else {
        result = cfg->registers[SPOC2_REGISTER_PCS] & SPOC2_4CHANNEL_PCS_PCCn_MASK;
    }

    switch (result) {
    case (uint8)SPOC2_PARALLEL_CHANNELS_NONE:
        enumResult = SPOC2_PARALLEL_CHANNELS_NONE;
        break;
    case (uint8)SPOC2_PARALLEL_CHANNELS_0_3:
        enumResult = SPOC2_PARALLEL_CHANNELS_0_3;
        break;
    case (uint8)SPOC2_PARALLEL_CHANNELS_1_2:
        enumResult = SPOC2_PARALLEL_CHANNELS_1_2;
        break;
    case (uint8)SPOC2_PARALLEL_CHANNELS_0_3_AND_1_2:
        enumResult = SPOC2_PARALLEL_CHANNELS_0_3_AND_1_2;
        break;

    // this case should never be reached
    default:
        enumResult = SPOC2_PARALLEL_CHANNELS_NONE;
        break;
    }

    return enumResult;
}

/// @brief Set the parallel channel configuration for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @param channels The configuration of parallel channels to enable
/// @return \ref SPOC2_ERROR_OK is successful, \ref SPOC2_ERROR_BAD_PARAMETER if the configuration is invalid for the
/// device
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_setParallelChannels
// Implements: S2DD-VD1_IPVSR-16
SPOC2_error_t SPOC2_Config_setParallelChannels(SPOC2_deviceConfig_t* cfg, SPOC2_parallelChannels_t channels) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    boolean is6Channel = SPOC2_deviceIs6Channel(*cfg); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                       // pointer must be verified by the API user"
    if (is6Channel) {
        switch (channels) {
        case SPOC2_PARALLEL_CHANNELS_NONE:
            cfg->registers[SPOC2_REGISTER_PCS] &= ~SPOC2_6CHANNEL_PCS_PCCn_MASK;
            break;
        case SPOC2_PARALLEL_CHANNELS_1_2:
            cfg->registers[SPOC2_REGISTER_PCS] |= SPOC2_6CHANNEL_PCS_PCCn_MASK;
            break;
        default:
            result = SPOC2_ERROR_BAD_PARAMETER;
            break;
        }
    } else {
        switch (channels) {
        case SPOC2_PARALLEL_CHANNELS_NONE:
        case SPOC2_PARALLEL_CHANNELS_0_3:
        case SPOC2_PARALLEL_CHANNELS_1_2:
        case SPOC2_PARALLEL_CHANNELS_0_3_AND_1_2:
            cfg->registers[SPOC2_REGISTER_PCS] =
                (cfg->registers[SPOC2_REGISTER_PCS] & ~SPOC2_4CHANNEL_PCS_PCCn_MASK) | (uint8)channels;
            break;
        default:
            result = SPOC2_ERROR_BAD_PARAMETER;
            break;
        }
    }

    return result;
}

/// @brief Clear all the latches and restart counters for the specified device
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_clearAllRestartCounters
SPOC2_error_t SPOC2_Config_clearAllRestartCounters(SPOC2_deviceConfig_t* cfg) {
    cfg->registers[SPOC2_REGISTER_HWCR] |= // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                           // pointer must be verified by the API user"

        SPOC2_HWCR_CLC_MASK;
    return SPOC2_ERROR_OK;
}

/// @brief Clear the latch and restart counter for the currently selected current sense channel
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return \ref SPOC2_ERROR_OK
/// @details **Note:** the configuration change made by this function will not be applied until the next call to \ref
/// SPOC2_Chain_applyDeviceConfigs
// Implements: SPOC2_Config_clearSelectedChannelRestartCounter
SPOC2_error_t SPOC2_Config_clearSelectedChannelRestartCounter(SPOC2_deviceConfig_t* cfg) {
    cfg->registers[SPOC2_REGISTER_PCS] |= // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                          // pointer must be verified by the API user"
        SPOC2_PCS_CLCS_MASK;
    return SPOC2_ERROR_OK;
}

/// @brief Write values to the specified register for a whole chain of devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @param reg The device register to write the data into
/// @param txBuffer Pointer to a byte buffer containing enough data to write a register to the whole chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function is provided to cover use-cases where \ref SPOC2_Chain_applyDeviceConfigs is inappropriate.
/// Care must be taken to ensure that the data written to the selected register is valid, otherwise the devices may
/// behave in unexpected ways. This function will perform 1 byte of communication per device in the chain when writing
/// to registers in bank 0 (DCR.SWR == 0), and 3 bytes per device in the chain for registers in bank 1 (DCR.SWR == 1).
/// ```c
/// uint8 txBuffer[16];
/// for (int i = 0; i < 16; i++) {
///     // enable channels 0-4
///     txBuffer[i] = SPOC2_WRITE_OUT_HEADER | 0b00001111u;
/// }
/// // `chain` is 16 devices long
/// SPOC2_error_t result = SPOC2_Chain_writeRegister(&chain, SPOC2_REGISTER_OUT, txBuffer);
/// ASSERT(result == SPOC2_ERROR_OK);
/// ```
/// @note The driver functions operate under the assumption that DCR.SWR is 0 when they are called. Care should be taken
/// to correctly reset DCR.SWR if \ref SPOC2_Chain_writeRegister is used with the DCR register.
// Implements: SPOC2_Chain_writeRegister
// Implements: S2DD-VD1_IPVSR-2
SPOC2_error_t SPOC2_Chain_writeRegister(SPOC2_chain_t* chain, SPOC2_register_t reg, uint8* txBuffer) {
    uint8 i;
    uint32 numDevices = chain->numDevices; // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                           // pointer must be verified by the API user"
    uint8 txBufferDCR[SPOC2_CHAIN_MAX_LEN];
    boolean register_bank_1 = (reg == SPOC2_REGISTER_RCD) || (reg == SPOC2_REGISTER_PCS) || (reg == SPOC2_REGISTER_ICS);
    SPOC2_error_t result = SPOC2_ERROR_OK;

    if (numDevices == 0) {
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    if (result == SPOC2_ERROR_OK) {
        // write the value into the shadow registers
        for (i = 0; i < numDevices; i++) {
            chain->devices[i].registers[reg] = txBuffer[i]; // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of
                                                            // the pointer must be verified by the API user"
            // prepare to switch register banks if necessary
            if (register_bank_1) {
                txBufferDCR[i] = SPOC2_WRITE_DCR_HEADER | SPOC2_DCR_SWR_MASK;
            }
        }
    }

    // select register bank 1 if necessary
    if ((result == SPOC2_ERROR_OK) && register_bank_1) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferDCR, NULL, numDevices);
    }
    // write the register to the chain
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer, NULL, numDevices);
    }
    // reset register bank to 0 if necessary
    if ((result == SPOC2_ERROR_OK) && register_bank_1) {
        for (i = 0; i < numDevices; i++) {
            txBufferDCR[i] = chain->devices[i].registers[SPOC2_REGISTER_DCR];
        }
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferDCR, NULL, numDevices);
    }
    return result;
}

/// @brief Reads values from the specified register for a whole chain of devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @param reg The device register to read the data from
/// @param rxBuffer Pointer to a byte buffer large enough to read a register from the whole chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function is provided to cover use-cases where the other provided API functions are inappropriate. This
/// function will perform 2 bytes of SPI communication per device in the chain.
/// ```c
/// uint8 rxBuffer[16];
/// // `chain` is 16 devices long
/// ASSERT(SPOC2_Chain_readRegister(&chain, SPOC2_REGISTER_HWCR, rxBuffer) == SPOC2_ERROR_OK);
/// ```
// Implements: SPOC2_Chain_readRegister
// Implements: S2DD-VD1_IPVSR-1
SPOC2_error_t SPOC2_Chain_readRegister(const SPOC2_chain_t* chain, SPOC2_register_t reg, uint8* rxBuffer) {
    uint8 txBuffer[SPOC2_CHAIN_MAX_LEN];
    uint8 dummyBuffer[SPOC2_CHAIN_MAX_LEN] = {
        0}; // all zeros represents a command to read the OUT register which is harmless
    uint8 i;
    uint8 readCommand;
    SPOC2_error_t result = SPOC2_ERROR_OK;

    if (chain->numDevices == 0) { // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                  // pointer must be verified by the API user"
        result = SPOC2_ERROR_BAD_PARAMETER;
    }
    switch (reg) {
    case SPOC2_REGISTER_OUT:
        readCommand = SPOC2_READ_OUT;
        break;
    case SPOC2_REGISTER_RCS:
        readCommand = SPOC2_READ_RCS;
        break;
    case SPOC2_REGISTER_SRC:
        readCommand = SPOC2_READ_SRC;
        break;
    case SPOC2_REGISTER_OCR:
        readCommand = SPOC2_READ_OCR;
        break;
    case SPOC2_REGISTER_RCD:
        readCommand = SPOC2_READ_RCD;
        break;
    case SPOC2_REGISTER_KRC:
        readCommand = SPOC2_READ_KRC;
        break;
    case SPOC2_REGISTER_PCS:
        readCommand = SPOC2_READ_PCS;
        break;
    case SPOC2_REGISTER_HWCR:
        readCommand = SPOC2_READ_KRC;
        break;
    case SPOC2_REGISTER_ICS:
        readCommand = SPOC2_READ_ICS;
        break;
    case SPOC2_REGISTER_DCR:
        readCommand = SPOC2_READ_DCR;
        break;
    default:
        result = SPOC2_ERROR_BAD_PARAMETER;
        break;
    }

    if (result == SPOC2_ERROR_OK) {
        for (i = 0; i < chain->numDevices; i++) {
            txBuffer[i] = readCommand;
        }

        // Write the read command out to the devices
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer, NULL, chain->numDevices);

        // Read the responses back from the devices into the user buffer
        if (result == SPOC2_ERROR_OK) {
            result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, dummyBuffer, rxBuffer,
                                                chain->numDevices);
        }
    }
    return result;
}

/// @brief Read the states of the input pins for a whole chain of devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @param rxBuffer Pointer to a byte buffer large enough to read a register from the whole chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This funtion internally calls \ref SPOC2_Chain_readRegister and so will perform 2 bytes of SPI
/// communication per device in the chain
// Implements: SPOC2_Chain_readInputStates
SPOC2_error_t SPOC2_Chain_readInputStates(const SPOC2_chain_t* chain, uint8* rxBuffer) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    uint8 i;

    result = SPOC2_Chain_readRegister(chain, SPOC2_REGISTER_ICS, rxBuffer);
    if (result == SPOC2_ERROR_OK) {
        for (i = 0; i < chain->numDevices; i++) { // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                  // pointer must be verified by the API user"
            rxBuffer[i] &= SPOC2_ICS_INSTn_MASK;  // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                                  // pointer must be verified by the API user"
        }
    }

    return result;
}

/// @brief Read the diagnostic registers from a whole chain of devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @param stdDiagBuffer STDDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @param wrnDiagBuffer WRNDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @param errDiagBuffer ERRDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details The diagnostic registers contain flags which indicate various events that can occur during operation.
/// Please refer to the device datasheet for details on the layout of these registers. This function will perform 4
/// bytes of SPI communication per device in the chain.
// Implements: SPOC2_Chain_readDiagnostics
SPOC2_error_t SPOC2_Chain_readDiagnostics(const SPOC2_chain_t* chain, uint8* stdDiagBuffer, uint8* wrnDiagBuffer,
                                          uint8* errDiagBuffer) {
    uint8 txBuffer1[SPOC2_CHAIN_MAX_LEN];
    uint8 txBuffer2[SPOC2_CHAIN_MAX_LEN];
    uint8 txBuffer3[SPOC2_CHAIN_MAX_LEN];
    uint8 i;
    uint32 numDevices = chain->numDevices; // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                           // pointer must be verified by the API user"

    SPOC2_error_t result = SPOC2_ERROR_OK;

    for (i = 0; i < numDevices; i++) {
        txBuffer1[i] = SPOC2_READ_STDDIAG;
        txBuffer2[i] = SPOC2_READ_WRNDIAG;
        txBuffer3[i] = SPOC2_READ_ERRDIAG;
    }

    // send STDDIAG read command, discard previous response
    result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer1, NULL, numDevices);

    // send WRNDIAG read command, read STDDIAG response
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer2, stdDiagBuffer, numDevices);
    } else {
    }
    // send ERRDIAG read command, read WRNDIAG response
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer3, wrnDiagBuffer, numDevices);
    } else {
    }
    // send duplicate ERRDIAG read command, read ERRDIAG response
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer3, errDiagBuffer, numDevices);
    } else {
    }

    return result;
}

/// @brief Apply the current shadow configurations to a whole chain of devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function should be called after all desired configuration changes are made to minimize unnecessary SPI
/// communication. This function performs 9 bytes of SPI communication per device in the chain.
// Implements: SPOC2_Chain_applyDeviceConfigs
// Implements: S2DD-VD1_IPVSR-3
SPOC2_error_t SPOC2_Chain_applyDeviceConfigs(SPOC2_chain_t* chain) {
    uint8 txBufferDCR[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferR1[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferR2[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferR3[SPOC2_CHAIN_MAX_LEN];
    uint8 i;
    uint8 rcdDataMask;
    uint8 pcsDataMask;
    uint8 ocrDataMask;
    uint8 krcDataMask;
    SPOC2_error_t result = SPOC2_ERROR_OK;

    if (chain->numDevices == 0) { // polyspace MISRA-C3:D4.14 [Justified:Low] ""
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    // write registers in bank 0
    for (i = 0; i < chain->numDevices; i++) {
        // clear the DCR.SWR bit to choose register bank 0
        // ignore the user-selected current sense channel, as this may put the device into sleep mode
        txBufferDCR[i] = SPOC2_WRITE_DCR_HEADER;

        // prepare data for HWCR register write
        txBufferR1[i] = SPOC2_WRITE_HWCR_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_HWCR] &
                                                   (SPOC2_HWCR_COL_MASK | SPOC2_HWCR_RST_MASK | SPOC2_HWCR_CLC_MASK));
        // clear the RST and CLC bits in the shadow HWCR register to prevent multiple resets
        chain->devices[i].registers[SPOC2_REGISTER_HWCR] &= ~(SPOC2_HWCR_RST_MASK | SPOC2_HWCR_CLC_MASK);

        // prepare data for OCR register write
        ocrDataMask =
            SPOC2_deviceIs6Channel(chain->devices[i]) ? SPOC2_6CHANNEL_OCR_OCTn_MASK : SPOC2_4CHANNEL_OCR_OCTn_MASK;
        txBufferR2[i] = SPOC2_WRITE_OCR_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_OCR] & ocrDataMask);

        // prepare data for KRC register write
        krcDataMask =
            SPOC2_deviceIs6Channel(chain->devices[i]) ? SPOC2_6CHANNEL_KRC_KRCn_MASK : SPOC2_4CHANNEL_KRC_KRCn_MASK;
        txBufferR3[i] = SPOC2_WRITE_KRC_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_KRC] & krcDataMask);
    }
    // write to DCR to select register bank 0
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferDCR[0], NULL, chain->numDevices);
    }
    // write to all device HWCR registers (this can trigger a device reset, so make sure it is done first)
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR1[0], NULL, chain->numDevices);
    }

    // write to all device OCR registers
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR2[0], NULL, chain->numDevices);
    }
    // write to all device KRC registers
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR3[0], NULL, chain->numDevices);
    }
    // write to registers in bank 1
    for (i = 0; i < chain->numDevices; i++) {
        // set the SWR bit to choose register bank 1
        // make sure the user's current sense channel is selected for PCS config
        txBufferDCR[i] = SPOC2_WRITE_DCR_HEADER | SPOC2_DCR_SWR_MASK |
                         (chain->devices[i].registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK);

        // prepare data for RCD register write
        rcdDataMask =
            SPOC2_deviceIs6Channel(chain->devices[i]) ? SPOC2_6CHANNEL_RCD_RCDn_MASK : SPOC2_4CHANNEL_RCD_RCDn_MASK;
        txBufferR1[i] = SPOC2_WRITE_RCD_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_RCD] & rcdDataMask);

        // prepare data for PCS register write
        pcsDataMask =
            SPOC2_deviceIs6Channel(chain->devices[i]) ? SPOC2_6CHANNEL_PCS_PCCn_MASK : SPOC2_4CHANNEL_PCS_PCCn_MASK;
        txBufferR2[i] = SPOC2_WRITE_PCS_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_PCS] &
                                                  (pcsDataMask | SPOC2_PCS_CLCS_MASK | SPOC2_PCS_SRCS_MASK));
        // clear the CLCS bit in the shadow PCS register to prevent multiple resets
        chain->devices[i].registers[SPOC2_REGISTER_PCS] &= ~(SPOC2_PCS_CLCS_MASK);

        // prepare buffer for OUT register write
        txBufferR3[i] = SPOC2_WRITE_OUT_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_OUT]);
    }

    // write to DCR to select register bank 1
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferDCR[0], NULL, chain->numDevices);
    }
    // write to all device RCD registers
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR1[0], NULL, chain->numDevices);
    }
    // write to all device PCS registers
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR2[0], NULL, chain->numDevices);
    }
    // write to the DCR register (selected current sense channel or sleep mode)
    // and the OUT register to switch the user-selected channels
    for (i = 0; i < chain->numDevices; i++) {
        // set DCR.SWR to 0 for consistency
        txBufferDCR[i] = (SPOC2_WRITE_DCR_HEADER & ~SPOC2_DCR_SWR_MASK) |
                         (chain->devices[i].registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK);
    }
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferDCR[0], NULL, chain->numDevices);
    }
    // write to all device OUT registers
    if (result == SPOC2_ERROR_OK) {
        result =
            SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferR3[0], NULL, chain->numDevices);
    }

    return result;
}

/// @brief Sends a reset signal to all devices in a chain and resets their shadow configurations
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details Unlike \ref SPOC2_Config_resetDevice, this function will perform SPI communication to immediately reset
/// devices in the chain, and can be used when a fault occurs, leaving the devices configured in an unknown state that
/// may not match the shadow configuration. This function performs 2 bytes of SPI communication per device in the chain.
// Implements: SPOC2_Chain_resetAllDevices
SPOC2_error_t SPOC2_Chain_resetAllDevices(SPOC2_chain_t* chain) {
    uint8 txBufferDCR[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferHWCR[SPOC2_CHAIN_MAX_LEN];
    uint8 i;
    SPOC2_error_t result = SPOC2_ERROR_OK;

    for (i = 0; i < chain->numDevices; i++) {
        (void)SPOC2_Config_resetDevice(&chain->devices[i]);

        // ensure DCR.SWR is set to 0 to allow writes to HWCR
        txBufferDCR[i] = SPOC2_WRITE_DCR_HEADER;

        // write to the reset bit in the HWCR register
        txBufferHWCR[i] = SPOC2_WRITE_HWCR_HEADER | SPOC2_HWCR_RST_MASK;
    }

    result =
        SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferDCR[0], NULL, chain->numDevices);
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, &txBufferHWCR[0], NULL,
                                            chain->numDevices);
    } else {
    }

    return result;
}

/// @brief Perform checksum verification of all device configurations in a chain
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_CHECKSUM_FAILURE if a checksum does not match, or
/// another error value if failed
/// @details The SPOC™ +2 family of devices can perform a simple checksum on a subset of the configured values to verify a
/// match with expected values. This function will calculate the checksum and perform 4 bytes of SPI communication per
/// device in the chain to verify that the shadow configuration registers match the device registers.
// Implements: SPOC2_Chain_verifyDeviceConfigs
// Implements: S2DD-VD1_IPVSR-13
SPOC2_error_t SPOC2_Chain_verifyDeviceConfigs(const SPOC2_chain_t* chain) {
    uint8 checksums[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferDCR[SPOC2_CHAIN_MAX_LEN];
    uint8 txBufferSDG[SPOC2_CHAIN_MAX_LEN];
    uint8 rxBuffer[SPOC2_CHAIN_MAX_LEN];
    uint8 data;
    uint32 i;
    uint32 numDevices = chain->numDevices; // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity of the
                                           // pointer must be verified by the API user"
    SPOC2_error_t result = SPOC2_ERROR_OK;
    SPOC2_error_t checksumResult = SPOC2_ERROR_OK;

    // Calculate checksums from shadow configs
    for (i = 0; i < numDevices; i++) {
        // set DCR.SWR to 1 to write to ICS
        txBufferDCR[i] =
            SPOC2_WRITE_DCR_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_DCR] | SPOC2_DCR_SWR_MASK);
        // read STDDIAG to see possible checksum failures
        txBufferSDG[i] = SPOC2_READ_STDDIAG;

        data = 0;
        if (SPOC2_deviceIs6Channel(chain->devices[i])) {
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_OCR] & SPOC2_6CHANNEL_OCR_OCTn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_RCD] & SPOC2_6CHANNEL_RCD_RCDn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_KRC] & SPOC2_6CHANNEL_KRC_KRCn_MASK);

            // the top 2 bits of SRCn need to be XOR-ed into the bottom data bit
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_SRC] & SPOC2_6CHANNEL_KRC_KRCn_MASK) & 0b00001111u;
            data ^=
                ((chain->devices[i].registers[SPOC2_REGISTER_SRC] & SPOC2_6CHANNEL_KRC_KRCn_MASK) >> 4) & 0b00001111u;
            data ^=
                ((chain->devices[i].registers[SPOC2_REGISTER_SRC] & SPOC2_6CHANNEL_KRC_KRCn_MASK) >> 5) & 0b00001111u;
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_HWCR] & SPOC2_HWCR_COL_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_PCS] & SPOC2_6CHANNEL_PCS_PCCn_MASK) >> 2;

            // the checksum is the nibble required to reach even-odd-even-odd parity in the nibble
            checksums[i] = SPOC2_WRITE_ICS_HEADER | ((data ^ 0b00000101u) & 0b00001111u);
        } else {
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_OCR] & SPOC2_4CHANNEL_OCR_OCTn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_RCD] & SPOC2_4CHANNEL_RCD_RCDn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_KRC] & SPOC2_4CHANNEL_KRC_KRCn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_SRC] & SPOC2_4CHANNEL_SRC_SRCn_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_HWCR] & SPOC2_HWCR_COL_MASK);
            data ^= (chain->devices[i].registers[SPOC2_REGISTER_PCS] & SPOC2_4CHANNEL_PCS_PCCn_MASK) >> 2;

            // the checksum is the nibble required to reach even-odd-even-odd parity in the nibble
            checksums[i] = SPOC2_WRITE_ICS_HEADER | ((data ^ 0b00000101u) & 0b00001111u);
        }
    }

    // Write to DCR.SWR
    result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferDCR, NULL, numDevices);
    // Write to ICS to begin the checksum calculation
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, checksums, NULL, numDevices);
    } else {
    }
    // Read the next response
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferSDG, rxBuffer, numDevices);
    } else {
    }
    // Check the next response for errors in STDDIAG frames if there is no transmission error
    if (result == SPOC2_ERROR_OK) {
        for (i = 0; i < numDevices; i++) {
            if (((rxBuffer[i] & 0b11000000u) == 0) && ((rxBuffer[i] & 0b00010000u) != 0)) {
                // Checksum verification error
                checksumResult = SPOC2_ERROR_CHECKSUM_FAILURE;
            }
        }
    }

    // Read another response for the STDDIAG frames of the devices that sent WRNDIAG last time
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferSDG, rxBuffer, numDevices);
    }
    // Check the next response for errors in STDDIAG frames if there is no transmission error
    if (result == SPOC2_ERROR_OK) {
        for (i = 0; i < numDevices; i++) {
            if (((rxBuffer[i] & 0b11000000u) == 0) && ((rxBuffer[i] & 0b00010000u) != 0)) {
                // Checksum verification error
                checksumResult = SPOC2_ERROR_CHECKSUM_FAILURE;
            }
        }
    }

    // Set DCR.SWR to 0 for consistency
    for (i = 0; i < numDevices; i++) {
        txBufferDCR[i] =
            SPOC2_WRITE_DCR_HEADER | (chain->devices[i].registers[SPOC2_REGISTER_DCR] & ~SPOC2_DCR_SWR_MASK);
    }
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBufferDCR, NULL, numDevices);
    }
    return (result != SPOC2_ERROR_OK) ? result : checksumResult;
}

/// @brief Read the current Millivolt value of the selected device's current sense pin
/// @param cfg Pointer to the SPOC™ +2 device configuration structure
/// @return The current signal level at the current sense pin in mV
/// @details To obtain a value for the current being measured through the selected channel, the user must use their
/// configured value of KILIS and the value of RSENSE from their application circuit to calculate the output current.
/// Please refer to the device datasheet for more details.
// Implements: SPOC2_readCurrentSense
// Implements: S2DD-VD1_IPVSR-5
uint32 SPOC2_readCurrentSense(const SPOC2_deviceConfig_t* cfg) {
    return SPOC2_ADC_readChannelMilliVolts(cfg->adcChannel); // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity
                                                             // of the pointer must be verified by the API user"
}

/// @brief Perform current sense verification for the specified device in the chain
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @param deviceIndex The index of the device in the chain's array of devices
/// @param expectedMv The expected value of the current sense pin in Millivolts
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_CURRENT_SENSE_VERIFICATION_FAILURE if the measured value
/// is not within tolerance of the expected value, or another error value if failed
/// @details The SPOC™ +2 family of devices can perform a current sense verification check, where a constant current of
/// 500uA is emitted from the current sense pin. In order to use this function the user must calculate the expected
/// Millivolt value generated across their application circuit's RSENSE by 500uA of current and provide this value as an
/// argument to the function
// Implements: SPOC2_verifyCurrentSense
SPOC2_error_t SPOC2_verifyCurrentSense(const SPOC2_chain_t* chain, uint8 deviceIndex, uint32 expectedMv) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
    uint8 txBuffer[SPOC2_CHAIN_MAX_LEN] = {0};
    uint32 adcAvg, adcLowerBound, adcUpperBound;
    boolean is6Channel;

    if (deviceIndex >= chain->numDevices) { // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity
                                            // of the pointer must be verified by the API user"
        result = SPOC2_ERROR_BAD_PARAMETER;
    } else {
        is6Channel = SPOC2_deviceIs6Channel(chain->devices[deviceIndex]);
        if (!is6Channel) {
            // Enable current sense verification mode for the desired device
            txBuffer[deviceIndex] = SPOC2_WRITE_DCR_HEADER | (0b00000101u);

            result =
                SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer, NULL, chain->numDevices);

            if (result == SPOC2_ERROR_OK) {
                // ADC read from channel specified in the device configuration
                adcAvg = SPOC2_ADC_readChannelMilliVolts(chain->devices[deviceIndex].adcChannel);
                adcAvg += SPOC2_ADC_readChannelMilliVolts(chain->devices[deviceIndex].adcChannel);
                adcAvg += SPOC2_ADC_readChannelMilliVolts(chain->devices[deviceIndex].adcChannel);

                adcAvg /= 3;

                // Set current sense channel back to how it was
                txBuffer[deviceIndex] =
                    SPOC2_WRITE_DCR_HEADER | chain->devices[deviceIndex].registers[SPOC2_REGISTER_DCR];
                result = SPOC2_SPI_transferBlocking(chain->spiBusId, chain->spiChipSelect, txBuffer, NULL,
                                                    chain->numDevices);
                adcLowerBound = (expectedMv * 4) / 5;
                adcUpperBound = (expectedMv * 6) / 5;
                if ((result == SPOC2_ERROR_OK) && !((adcAvg >= adcLowerBound) && (adcAvg <= adcUpperBound))) {
                    // return error if they don't match within tolerance
                    result = SPOC2_ERROR_CURRENT_SENSE_VERIFICATION_FAILURE;
                }
            }
        } else {
            result = SPOC2_ERROR_INVALID_OPERATION;
        }
    }

    return result;
}

/// @brief Initialize a chain of SPOC™ +2 devices
/// @param chain Pointer to the SPOC™ +2 chain structure
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the chain is configured with too many
/// devices, or another error value if failed
/// @details This function must be called on the chain before attempting to apply any configuration, and will reset the
/// devices and the shadow registers into a known state.
// Implements: SPOC2_Chain_init
// Implements: S2DD-VD1_IPVSR-4
SPOC2_error_t SPOC2_Chain_init(SPOC2_chain_t* chain) {
    SPOC2_error_t result = SPOC2_ERROR_OK;

    if (chain->numDevices > SPOC2_CHAIN_MAX_LEN) { // polyspace MISRA-C3:D4.14 [Justified:Low] "The validity
                                                   // of the pointer must be verified by the API user"
        result = SPOC2_ERROR_BAD_PARAMETER;
    }

    // send a reset signal to the whole chain to clear any old configuration and diagnostics
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_Chain_resetAllDevices(chain);
    }

    return result;
}

/// @brief Write values to the specified register for a single device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @param reg The device register to write the data into
/// @param txBuffer Pointer to a byte buffer containing enough data to write a register
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function is provided to cover use-cases where \ref SPOC2_applyDeviceConfigs is inappropriate.
SPOC2_error_t SPOC2_writeRegister(SPOC2_deviceConfig_t* dev, SPOC2_register_t reg, uint8* txBuffer) {
    uint8 txBufferDCR;
    boolean register_bank_1 = (reg == SPOC2_REGISTER_RCD) || (reg == SPOC2_REGISTER_PCS) || (reg == SPOC2_REGISTER_ICS);
    SPOC2_error_t result = SPOC2_ERROR_OK;

    if (result == SPOC2_ERROR_OK) {
        // write the value into the shadow registers
        dev->registers[reg] = *txBuffer; 

        // prepare to switch register banks if necessary
        if (register_bank_1) {
            txBufferDCR = SPOC2_WRITE_DCR_HEADER | SPOC2_DCR_SWR_MASK;
        }
    }

    // select register bank 1 if necessary
    if ((result == SPOC2_ERROR_OK) && register_bank_1) {
        result = SPOC2_SPI_transferBlocking2(dev->id, txBuffer, NULL, 1);
    }
    // write the register to the chain
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, txBuffer, NULL, 1);
    }
    // reset register bank to 0 if necessary
    if ((result == SPOC2_ERROR_OK) && register_bank_1) {
        txBufferDCR =  dev->registers[SPOC2_REGISTER_DCR];
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferDCR, NULL, 1);
    }
    return result;
}

/// @brief Reads values from the specified register for a single device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @param reg The device register to read the data from
/// @param rxBuffer Pointer to a byte buffer large enough to read a register
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function is provided to cover use-cases where the other provided API functions are inappropriate. This
/// function will perform 2 bytes of SPI communication per device.
SPOC2_error_t SPOC2_readRegister(const SPOC2_deviceConfig_t* dev, SPOC2_register_t reg, uint8* rxBuffer) {
    uint8 txBuffer;
    uint8 dummyBuffer = 0;
    uint8 readCommand;
    SPOC2_error_t result = SPOC2_ERROR_OK;


    switch (reg) {
    case SPOC2_REGISTER_OUT:
        readCommand = SPOC2_READ_OUT;
        break;
    case SPOC2_REGISTER_RCS:
        readCommand = SPOC2_READ_RCS;
        break;
    case SPOC2_REGISTER_SRC:
        readCommand = SPOC2_READ_SRC;
        break;
    case SPOC2_REGISTER_OCR:
        readCommand = SPOC2_READ_OCR;
        break;
    case SPOC2_REGISTER_RCD:
        readCommand = SPOC2_READ_RCD;
        break;
    case SPOC2_REGISTER_KRC:
        readCommand = SPOC2_READ_KRC;
        break;
    case SPOC2_REGISTER_PCS:
        readCommand = SPOC2_READ_PCS;
        break;
    case SPOC2_REGISTER_HWCR:
        readCommand = SPOC2_READ_KRC;
        break;
    case SPOC2_REGISTER_ICS:
        readCommand = SPOC2_READ_ICS;
        break;
    case SPOC2_REGISTER_DCR:
        readCommand = SPOC2_READ_DCR;
        break;
    default:
        result = SPOC2_ERROR_BAD_PARAMETER;
        break;
    }

    if (result == SPOC2_ERROR_OK) {
        txBuffer = readCommand;

        // Write the read command out to the devices
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBuffer, NULL, 1);

        // Read the responses back from the devices into the user buffer
        if (result == SPOC2_ERROR_OK) {
            result = SPOC2_SPI_transferBlocking2(dev->id, &dummyBuffer, rxBuffer, 1);
        }
    }
    return result;
}

/// @brief Read the states of the input pins for a single device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @param rxBuffer Pointer to a byte buffer large enough to read a register from the whole chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This funtion internally calls \ref SPOC2_readRegister and so will perform 2 bytes of SPI
/// communication per device in the chain
// Implements: SPOC2_readInputStates
SPOC2_error_t SPOC2_readInputStates(const SPOC2_deviceConfig_t* dev, uint8* rxBuffer) {
    SPOC2_error_t result = SPOC2_ERROR_OK;

    result = SPOC2_readRegister(dev, SPOC2_REGISTER_ICS, rxBuffer);

    if (result == SPOC2_ERROR_OK) {
        *rxBuffer &= SPOC2_ICS_INSTn_MASK;
    }

    return result;
}

/// @brief Read the diagnostic registers from a single device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @param stdDiagBuffer STDDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @param wrnDiagBuffer WRNDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @param errDiagBuffer ERRDIAG output buffer, pointer to a byte buffer large enough to read a register from the whole
/// chain
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details The diagnostic registers contain flags which indicate various events that can occur during operation.
/// Please refer to the device datasheet for details on the layout of these registers. This function will perform 4
/// bytes of SPI communication per device.
// Implements: SPOC2_Chain_readDiagnostics
SPOC2_error_t SPOC2_readDiagnostics(const SPOC2_deviceConfig_t* dev, uint8* stdDiagBuffer, uint8* wrnDiagBuffer,
                                          uint8* errDiagBuffer) {
    uint8 txBuffer1;
    uint8 txBuffer2;
    uint8 txBuffer3;

    SPOC2_error_t result = SPOC2_ERROR_OK;

    txBuffer1 = SPOC2_READ_STDDIAG;
    txBuffer2 = SPOC2_READ_WRNDIAG;
    txBuffer3 = SPOC2_READ_ERRDIAG;

    // send STDDIAG read command, discard previous response
    result = SPOC2_SPI_transferBlocking2(dev->id, &txBuffer1, NULL, 1);

    // send WRNDIAG read command, read STDDIAG response
    if (result == SPOC2_ERROR_OK)
    {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBuffer2, stdDiagBuffer, 1);
    }
    else
    {
    }
    // send ERRDIAG read command, read WRNDIAG response
    if (result == SPOC2_ERROR_OK)
    {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBuffer3, wrnDiagBuffer, 1);
    }
    else
    {
    }
    // send duplicate ERRDIAG read command, read ERRDIAG response
    if (result == SPOC2_ERROR_OK)
    {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBuffer3, errDiagBuffer, 1);
    }
    else
    {
    }

    return result;
}

/// @brief Apply the current shadow configurations to a selected device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details This function should be called after all desired configuration changes are made to minimize unnecessary SPI
/// communication. This function performs 9 bytes of SPI communication per device.
// Implements: SPOC2_applyDeviceConfigs
// Implements: S2DD-VD1_IPVSR-3
SPOC2_error_t SPOC2_applyDeviceConfig(SPOC2_deviceConfig_t* dev) {
    uint8 txBufferDCR;
    uint8 txBufferR1;
    uint8 txBufferR2;
    uint8 txBufferR3;
    uint8 rcdDataMask;
    uint8 pcsDataMask;
    uint8 ocrDataMask;
    uint8 krcDataMask;
    SPOC2_error_t result = SPOC2_ERROR_OK;

    // write registers in bank 0

    // clear the DCR.SWR bit to choose register bank 0
    // ignore the user-selected current sense channel, as this may put the device into sleep mode
    txBufferDCR = SPOC2_WRITE_DCR_HEADER;

    // prepare data for HWCR register write
    txBufferR1 = SPOC2_WRITE_HWCR_HEADER | (dev->registers[SPOC2_REGISTER_HWCR] &
                                                (SPOC2_HWCR_COL_MASK | SPOC2_HWCR_RST_MASK | SPOC2_HWCR_CLC_MASK));
    // clear the RST and CLC bits in the shadow HWCR register to prevent multiple resets
    dev->registers[SPOC2_REGISTER_HWCR] &= ~(SPOC2_HWCR_RST_MASK | SPOC2_HWCR_CLC_MASK);

    // prepare data for OCR register write
    ocrDataMask =
        SPOC2_deviceIs6Channel(*dev) ? SPOC2_6CHANNEL_OCR_OCTn_MASK : SPOC2_4CHANNEL_OCR_OCTn_MASK;
    txBufferR2 = SPOC2_WRITE_OCR_HEADER | (dev->registers[SPOC2_REGISTER_OCR] & ocrDataMask);

    // prepare data for KRC register write
    krcDataMask =
        SPOC2_deviceIs6Channel(*dev) ? SPOC2_6CHANNEL_KRC_KRCn_MASK : SPOC2_4CHANNEL_KRC_KRCn_MASK;
    txBufferR3 = SPOC2_WRITE_KRC_HEADER | (dev->registers[SPOC2_REGISTER_KRC] & krcDataMask);

    // write to DCR to select register bank 0
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferDCR, NULL, 1);
    }
    // write to all device HWCR registers (this can trigger a device reset, so make sure it is done first)
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferR1, NULL, 1); 
    }

    // write to all device OCR registers
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferR2, NULL, 1);
    }
    // write to all device KRC registers
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferR3, NULL, 1);
    }
    // write to registers in bank 1
    // set the SWR bit to choose register bank 1
    // make sure the user's current sense channel is selected for PCS config
    txBufferDCR = SPOC2_WRITE_DCR_HEADER | SPOC2_DCR_SWR_MASK |
                        (dev->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK);

    // prepare data for RCD register write
    rcdDataMask =
        SPOC2_deviceIs6Channel(*dev) ? SPOC2_6CHANNEL_RCD_RCDn_MASK : SPOC2_4CHANNEL_RCD_RCDn_MASK;
    txBufferR1 = SPOC2_WRITE_RCD_HEADER | (dev->registers[SPOC2_REGISTER_RCD] & rcdDataMask);

    // prepare data for PCS register write
    pcsDataMask =
        SPOC2_deviceIs6Channel(*dev) ? SPOC2_6CHANNEL_PCS_PCCn_MASK : SPOC2_4CHANNEL_PCS_PCCn_MASK;
    txBufferR2 = SPOC2_WRITE_PCS_HEADER | (dev->registers[SPOC2_REGISTER_PCS] &
                                                (pcsDataMask | SPOC2_PCS_CLCS_MASK | SPOC2_PCS_SRCS_MASK));
    // clear the CLCS bit in the shadow PCS register to prevent multiple resets
    dev->registers[SPOC2_REGISTER_PCS] &= ~(SPOC2_PCS_CLCS_MASK);

    // prepare buffer for OUT register write
    txBufferR3 = SPOC2_WRITE_OUT_HEADER | (dev->registers[SPOC2_REGISTER_OUT]);

    // write to DCR to select register bank 1
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferDCR, NULL, 1);
    }
    // write to all device RCD registers
    if (result == SPOC2_ERROR_OK) {
        result = SPOC2_SPI_transferBlocking2(dev->id, &txBufferR1, NULL, 1);
    }
    // write to all device PCS registers
    if (result == SPOC2_ERROR_OK) {
        SPOC2_SPI_transferBlocking2(dev->id, &txBufferR2, NULL, 1);
    }
    // write to the DCR register (selected current sense channel or sleep mode)
    // and the OUT register to switch the user-selected channels
    // set DCR.SWR to 0 for consistency
    txBufferDCR = (SPOC2_WRITE_DCR_HEADER & ~SPOC2_DCR_SWR_MASK) |
                        (dev->registers[SPOC2_REGISTER_DCR] & SPOC2_DCR_MUX_MASK);

    if (result == SPOC2_ERROR_OK) {
        SPOC2_SPI_transferBlocking2(dev->id, &txBufferDCR, NULL, 1);
    }
    // write to all device OUT registers
    if (result == SPOC2_ERROR_OK) {
        SPOC2_SPI_transferBlocking2(dev->id, &txBufferR3, NULL, 1);
    }

    return result;
}

/// @brief Sends a reset signal to a selected device and resets their shadow configurations
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @return \ref SPOC2_ERROR_OK if successful, or an error value if failed
/// @details Unlike \ref SPOC2_Config_resetDevice, this function will perform SPI communication to immediately reset
/// devices in the chain, and can be used when a fault occurs, leaving the devices configured in an unknown state that
/// may not match the shadow configuration. This function performs 2 bytes of SPI communication per device.
// Implements: SPOC2_resetDevice
SPOC2_error_t SPOC2_resetDevice(SPOC2_deviceConfig_t* dev) {
    uint8 txBufferDCR;
    uint8 txBufferHWCR;
    SPOC2_error_t result = SPOC2_ERROR_OK;
    
    (void)SPOC2_Config_resetDevice(dev);

    // ensure DCR.SWR is set to 0 to allow writes to HWCR
    txBufferDCR = SPOC2_WRITE_DCR_HEADER;

    // write to the reset bit in the HWCR register
    txBufferHWCR = SPOC2_WRITE_HWCR_HEADER | SPOC2_HWCR_RST_MASK;

    SPOC2_SPI_transferBlocking2(dev->id, &txBufferDCR, NULL, 1);

    if (result == SPOC2_ERROR_OK) 
    {
        SPOC2_SPI_transferBlocking2(dev->id, &txBufferHWCR, NULL, 1);
    } else
    {
    }

    return result;
}

/// @brief Initialize a single SPOC™ +2 device
/// @param dev Pointer to the SPOC™ +2 device configuration
/// @return \ref SPOC2_ERROR_OK if successful, \ref SPOC2_ERROR_BAD_PARAMETER if the chain is configured with too many
/// devices, or another error value if failed
/// @details This function must be called on the chain before attempting to apply any configuration, and will reset the
/// devices and the shadow registers into a known state.
// Implements: SPOC2_init
// Implements: S2DD-VD1_IPVSR-4
SPOC2_error_t SPOC2_init(SPOC2_deviceConfig_t* dev) {
    SPOC2_error_t result = SPOC2_ERROR_OK;
                          
    result = SPOC2_ERROR_BAD_PARAMETER;

    // send a reset signal to the whole chain to clear any old configuration and diagnostics
    if (result == SPOC2_ERROR_OK) 
    {
        result = SPOC2_resetDevice(dev);
    }

    return result;
}

boolean SPOC2_SetState()
{
    return false;
}

/// @}