// Implements: SPOC2_Definitions
#ifndef SPOC2_DEFINITIONS_H_
#define SPOC2_DEFINITIONS_H_

/// @addtogroup defs Definitions
/// @{

// Number of devices used on bus
#define SPOC2_NUM_OF_DEVICES 2

// Devices id enum for SPOC2
typedef enum
{
    SPOC2_ID_1 = 0x00,
    SPOC2_ID_2 = 0x01,
    SPOC2_ID_MAX
}T_SPOC2_ID;

// Registers in bank 0
/// @brief Message header to write data into the OCR register (bank 0)
#define SPOC2_WRITE_OCR_HEADER ((uint8)0b11000000)
/// @brief Message header to write data into the KRC register (bank 0)
#define SPOC2_WRITE_KRC_HEADER ((uint8)0b11010000)
/// @brief Message header to write data into the HWCR register (bank 0)
#define SPOC2_WRITE_HWCR_HEADER ((uint8)0b11100000)

// Regsiters in bank 1
/// @brief Message header to write data into the RCD register (bank 1)
#define SPOC2_WRITE_RCD_HEADER ((uint8)0b11000000)
/// @brief Message header to write data into the PCS register (bank 1)
#define SPOC2_WRITE_PCS_HEADER ((uint8)0b11010000)
/// @brief Message header to write data into the ICS register (bank 1)
#define SPOC2_WRITE_ICS_HEADER ((uint8)0b11100000)

// Registers in both banks
/// @brief Message header to write data into the OUT register (bank 0 or bank 1)
#define SPOC2_WRITE_OUT_HEADER ((uint8)0b10000000)
/// @brief Message header to write data into the DCR register (bank 0 or bank 1)
#define SPOC2_WRITE_DCR_HEADER ((uint8)0b11110000)

// Register read commands
/// @brief Message header to read data from the OUT register
#define SPOC2_READ_OUT ((uint8)0b00000000)
/// @brief Message header to read data from the RCS register
#define SPOC2_READ_RCS ((uint8)0b00001000)
/// @brief Message header to read data from the SRC register
#define SPOC2_READ_SRC ((uint8)0b00001001)
/// @brief Message header to read data from the OCR register
#define SPOC2_READ_OCR ((uint8)0b00000100)
/// @brief Message header to read data from the RCD register
#define SPOC2_READ_RCD ((uint8)0b00001100)
/// @brief Message header to read data from the KRC register
#define SPOC2_READ_KRC ((uint8)0b00000101)
/// @brief Message header to read data from the PCS register
#define SPOC2_READ_PCS ((uint8)0b00001101)
/// @brief Message header to read data from the HWCR register
#define SPOC2_READ_HWCR ((uint8)0b00000110)
/// @brief Message header to read data from the ICS register
#define SPOC2_READ_ICS ((uint8)0b00001110)
/// @brief Message header to read data from the DCR register
#define SPOC2_READ_DCR ((uint8)0b00000111)

// Diagnosis register read commands
/// @brief Message header to read data from the STDDIAG register
#define SPOC2_READ_STDDIAG ((uint8) 0b00000010)
/// @brief Message header to read data from the WRNDIAG register
#define SPOC2_READ_WRNDIAG ((uint8) 0b00000001)
/// @brief Message header to read data from the ERRDIAG register
#define SPOC2_READ_ERRDIAG ((uint8) 0b00000011)

// Register field masks
/// @brief Bitmask of the OUTn field within the OUT register for 4-channel devices
#define SPOC2_4CHANNEL_OUT_OUTn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the OUTn field within the OUT register for 6-channel devices
#define SPOC2_6CHANNEL_OUT_OUTn_MASK ((uint8)0b00111111)
/// @brief Bitmask of the OCTn field within the OCR register for 4-channel devices
#define SPOC2_4CHANNEL_OCR_OCTn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the OCTn field within the OCR register for 6-channel devices
#define SPOC2_6CHANNEL_OCR_OCTn_MASK ((uint8)0b00001110)
/// @brief Bitmask of the RCDn field within the RCD register for 4-channel devices
#define SPOC2_4CHANNEL_RCD_RCDn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the RCDn field within the RCD register for 6-channel devices
#define SPOC2_6CHANNEL_RCD_RCDn_MASK ((uint8)0b00001110)
/// @brief Bitmask of the KRCn field within the KRC register for 4-channel devices
#define SPOC2_4CHANNEL_KRC_KRCn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the KRCn field within the KRC register for 6-channel devices
#define SPOC2_6CHANNEL_KRC_KRCn_MASK ((uint8)0b00001110)
/// @brief Bitmask of the PCCn field within the PCS register for 4-channel devices
#define SPOC2_4CHANNEL_PCS_PCCn_MASK ((uint8)0b00001100)
/// @brief Bitmask of the PCCn field within the PCS register for 6-channel devices
#define SPOC2_6CHANNEL_PCS_PCCn_MASK ((uint8)0b00001000)
/// @brief Bitmask of the SRCn field within the SRC register for 4-channel devices
#define SPOC2_4CHANNEL_SRC_SRCn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the SRCn field within the SRC register for 6-channel devices
#define SPOC2_6CHANNEL_SRC_SRCn_MASK ((uint8)0b00111111)
/// @brief Bitmask of the CLCS field within the PCS register
#define SPOC2_PCS_CLCS_MASK ((uint8)0b00000010)
/// @brief Bitmask of the SRCS field within the PCS register
#define SPOC2_PCS_SRCS_MASK ((uint8)0b00000001)
/// @brief Bitmask of the COL field within the HWCR register
#define SPOC2_HWCR_COL_MASK ((uint8)0b00000100)
/// @brief Bitmask of the RST field within the HWCR register
#define SPOC2_HWCR_RST_MASK ((uint8)0b00000010)
/// @brief Bitmask of the SLP field within the HWCR register
#define SPOC2_HWCR_SLP_MASK ((uint8)0b00000010)
/// @brief Bitmask of the CLC field within the HWCR register
#define SPOC2_HWCR_CLC_MASK ((uint8)0b00000001)
/// @brief Bitmask of the CRSn field within the ICS register
#define SPOC2_ICS_CRSn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the INSTn field within the ICS register
#define SPOC2_ICS_INSTn_MASK ((uint8)0b00001111)
/// @brief Bitmask of the SWR field within the DCR register
#define SPOC2_DCR_SWR_MASK ((uint8)0b00001000)
/// @brief Bitmask of the MUX field within the DCR register
#define SPOC2_DCR_MUX_MASK ((uint8)0b00000111)

/// @}

#endif // SPOC2_DEFINITIONS_H_