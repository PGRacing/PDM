#include "can_handler.h"
#include "cmsis_os2.h"
#include "typedefs.h"
#include "logger.h"
#include "string.h"
#include "semphr.h"
#include "bsp_caninput.h"
#include "app_isotp.h"
#include "pdm.h"
#include "config.h"
#include "tim.h"

#define CANH_FILTER_INSTANCE1_FIRST_BANK 0
#define CANH_FILTER_INSTANCE1_LAST_BANK 13
#define CANH_FILTER_INSTANCE2_FIRST_BANK 14
#define CANH_FILTER_INSTANCE2_LAST_BANK 27

#define CANH_TX_DEFAULT_BYTE 0xAA

// GIT hash commit filled in Makefile
#ifndef GIT_HASH
#define GIT_HASH "0000000"
#endif

// FW revision filled in Makefile
#ifndef FW_REVISION_MAJOR
#define FW_REVISION_MAJOR 0
#endif

#ifndef FW_REVISION_MINOR
#define FW_REVISION_MINOR 0
#endif

#ifndef FW_REVISION_PATCH
#define FW_REVISION_PATCH 0
#endif


T_CANH_CFG cansCfg[CANH_INSTANCE_MAX] = 
{
    [CANH_INSTANCE_1] = 
    {
        .enabled = TRUE,
        .baud = 1000000,
        .terminator = FALSE, 
        .baseId = 0x400
    },
    [CANH_INSTANCE_2] = 
    {
        .enabled = TRUE,
        .baud = 1000000,
        .terminator = TRUE,
        .baseId = 0x400
    },
};

T_CANH_REG cansReg[CANH_INSTANCE_MAX] = 
{
    [CANH_INSTANCE_1] = 
    {
        .hcan = &hcan1,
        .txQueueHandle = NULL,
        .rxQueueHandle = NULL,
        .readyForTx = FALSE,
        .txMailbox = {0}
    },
    [CANH_INSTANCE_2] = 
    {
        .hcan = &hcan2,
        .txQueueHandle = NULL,
        .rxQueueHandle = NULL,
        .readyForTx = FALSE,
        .txMailbox = {0}
    },
};

// Use only for debug!!!
#ifdef DEBUG
__unused static CAN_FilterTypeDef canAllowAllFilter = 
{
    .FilterBank = 0,
    .FilterMode = CAN_FILTERMODE_IDMASK,
    .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterIdHigh = 0x0000,
    .FilterIdLow = 0x0000,
    .FilterMaskIdHigh = 0x0000,
    .FilterMaskIdLow = 0x0000,
    .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterActivation = ENABLE,
    .SlaveStartFilterBank = 14
};
#endif

/////////////////////////
//////// CAN TX /////////
/////////////////////////
/* All CAN TX structures should be added here*/

/// @brief All used CAN bus ID's
typedef enum
{
    CANH_ID_TX_PNP               = 0x000,  // Header sent on device power-up
    CANH_ID_TX_SYS_STATUS        = 0x001,  // System status
    CANH_ID_TX_STATUS_1_8        = 0x002,  // Status of channels 1-8 (default value)
    CANH_ID_TX_STATUS_9_16       = 0x003,  // Status of channels 9-16 (default value)
    CANH_ID_TX_STATE_1_16        = 0x004,  // State of channels 1-16 (default value)
    CANH_ID_TX_VOLTAGE_1_4       = 0x005,  // Voltage of channels 1-4 (default value)
    CANH_ID_TX_VOLTAGE_5_8       = 0x006,  // Voltage of channels 5-8 (default value)
    CANH_ID_TX_VOLTAGE_9_12      = 0x007,  // Voltage of channels 9-12 (default value)
    CANH_ID_TX_VOLTAGE_13_16     = 0x008,  // Voltage of channels 13-16 (default value)
    CANH_ID_TX_CURRENT_1_4       = 0x009,  // Current of channels 1-4 (default value)
    CANH_ID_TX_CURRENT_5_8       = 0x00A,  // Current of channels 5-8 (default value)
    CANH_ID_TX_CURRENT_9_12      = 0x00B,  // Current of channels 9-12 (default value)
    CANH_ID_TX_CURRENT_13_16     = 0x00C,  // Current of channels 13-16 (default value)
    CANH_ID_TX_NAMES             = 0x00D,  // Channel of current name,
    CANH_ID_TX_PHY_INPUTS_1_4    = 0x00E,  // Physical inputs 1-4
    CANH_ID_TX_PHY_INPUTS_5_8    = 0x00F,  // Physical inputs 5-8
    CANH_ID_TX_IMU_ACC           = 0x010,  // IMU acceleration data
    CANH_ID_TX_IMU_GYRO          = 0x011,  // IMU gyroscope data
    CANH_ID_TX_CURRENT_RMS_1_4   = 0x012,  // RMS Current of channels 1-4 (default value)
    CANH_ID_TX_CURRENT_RMS_5_8   = 0x013,  // RMS Current of channels 1-4 (default value)
    CANH_ID_TX_I2T_HEAT_1_8      = 0x014,  // I2t heat of channels 1-8
    CANH_ID_TX_SOC_TRESH_1_4     = 0x015,  // SOC treshold of channels 1-4
    CANH_ID_TX_SOC_TRESH_5_8     = 0x016,  // SOC treshold of channels 5-8
    CANH_ID_TX_PWM_DUTY_1_8      = 0x017,  // PWM duty of channels 1-8
    CANH_ID_TX_DEV_DIAG          = 0x018,  // Device diagnostics
    CANH_ID_TX_FW_COMMIT_HASH    = 0x019,  // Firmware commit hash response

    // CONFIG PROTOCOL
    CANH_ID_RX_ISOTP_ENTRANCE    = 0x050,  // Entrance gateway for ISO-TP communication mode
    CANH_ID_TX_ISOTP             = 0x051,  // CAN ID used for ISO-TP transmission
    CANH_ID_RX_ISOTP             = 0x052,  // CAN ID used for ISO-TP reception
    CANH_ID_RX_HASH_REQ          = 0x053,  // CAN ID used for config hash request from desktop
    CANH_ID_TX_HASH_RESP         = 0x054,  // CAN ID used for config hash response to desktop
    CANH_ID_RX_CONFIG_REQ        = 0x055,  // CAN ID used for configuration request from desktop (eg. to send current config or request config update)

    // FORCE RESET
    CANH_ID_RX_FORCE_RESET_GW    = 0x0AF,  // Force reset command, if specific pattern is sent to CAN with this ID then device will reset  

}T_CANH_ID;

static bool CANH_InitFilterInstance1(void);
static bool CANH_InitFilterInstance2(void);

/// @brief [pnpTxMsg] Message sent on device power-up
T_CANH_TX_PACKAGE CANH_TxPnp =
{
    .header = 
    {
        .DLC = 3,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_PNP,
        .TransmitGlobalTime = DISABLE,
    },
    .data = {FW_REVISION_MAJOR, FW_REVISION_MINOR, FW_REVISION_PATCH},
};

#ifdef DEBUG
/// @brief [fwCommitHashTxMsg] Message sent on device power-up
T_CANH_TX_PACKAGE CANH_TxFwCommitHash =
{
    .header = 
    {
        .DLC = 7,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_FW_COMMIT_HASH,
        .TransmitGlobalTime = DISABLE,
    },
    .data = {GIT_HASH[0], GIT_HASH[1], GIT_HASH[2], GIT_HASH[3], GIT_HASH[4], GIT_HASH[5], GIT_HASH[6]},
};
#endif

T_CANH_TX_PACKAGE CANH_TxStatus1_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_STATUS_1_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};


T_CANH_TX_PACKAGE CANH_TxStatus9_16 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_STATUS_9_16,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxVoltage1_4 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_VOLTAGE_1_4,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxVoltage5_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_VOLTAGE_5_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxVoltage9_12 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_VOLTAGE_9_12,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxVoltage13_16 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_VOLTAGE_13_16,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrent1_4 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_1_4,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrent5_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_5_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrent9_12 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_9_12,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrent13_16 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_13_16,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxSysStatus = 
{
    .header = 
    {
        .DLC = sizeof(T_CANH_SYSTEM_STATUS),
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_SYS_STATUS,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxState1_16 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_STATE_1_16,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxNames = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_NAMES,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxPhyInputs1_4 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_PHY_INPUTS_1_4,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxPhyInputs5_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_PHY_INPUTS_5_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxImuAcc = 
{
    .header = 
    {
        .DLC = 6,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_IMU_ACC,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxImuGyro = 
{
    .header = 
    {
        .DLC = 6,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_IMU_GYRO,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrentRMS1_4 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_RMS_1_4,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxCurrentRMS5_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_CURRENT_RMS_5_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxI2tHeat1_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_I2T_HEAT_1_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxSOCThreshold1_4 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_SOC_TRESH_1_4,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxSOCThreshold5_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_SOC_TRESH_5_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxHashResp = 
{
    .header = 
    {
        .DLC = 4,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_TX_HASH_RESP,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxPWMDuty = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId =   CANH_ID_TX_PWM_DUTY_1_8,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

T_CANH_TX_PACKAGE CANH_TxDevDiag = 
{
    .header = 
    {
        .DLC = 1,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId =   CANH_ID_TX_DEV_DIAG,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};


extern void RTOS_SuspendCAN_1(void);
extern void RTOS_SuspendCAN_2(void);
extern void RTOS_ResumeCAN_1(void);
extern void RTOS_ResumeCAN_2(void);

void CANH_Send_TxStatus1_8(T_CANH_INSTANCE instance, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8)
{   
    CANH_TxStatus1_8.data.status_8ch.status[0] = s1;
    CANH_TxStatus1_8.data.status_8ch.status[1] = s2;
    CANH_TxStatus1_8.data.status_8ch.status[2] = s3;
    CANH_TxStatus1_8.data.status_8ch.status[3] = s4;
    CANH_TxStatus1_8.data.status_8ch.status[4] = s5;
    CANH_TxStatus1_8.data.status_8ch.status[5] = s6;
    CANH_TxStatus1_8.data.status_8ch.status[6] = s7;
    CANH_TxStatus1_8.data.status_8ch.status[7] = s8;

    CANH_PushToTxQueue(instance, CANH_TxStatus1_8);
}

void CANH_Send_TxStatus9_16(T_CANH_INSTANCE instance, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8)
{   
    CANH_TxStatus9_16.data.status_8ch.status[0] = s1;
    CANH_TxStatus9_16.data.status_8ch.status[1] = s2;
    CANH_TxStatus9_16.data.status_8ch.status[2] = s3;
    CANH_TxStatus9_16.data.status_8ch.status[3] = s4;
    CANH_TxStatus9_16.data.status_8ch.status[4] = s5;
    CANH_TxStatus9_16.data.status_8ch.status[5] = s6;
    CANH_TxStatus9_16.data.status_8ch.status[6] = s7;
    CANH_TxStatus9_16.data.status_8ch.status[7] = s8;

    CANH_PushToTxQueue(instance, CANH_TxStatus9_16);
}

void CANH_Send_TxState1_16(T_CANH_INSTANCE instance, uint8_t s[16])
{
    for(uint8_t i = 0; i < 8; i++)
    {
        CANH_TxState1_16.data.state_16ch.state[i] = 0;
    }

    for(uint8_t i = 0; i < 16; i++)
    {
        // Pack states to one 8 byte frame
        CANH_TxState1_16.data.state_16ch.state[i / 2] |= s[i] << (i % 2 ? 0 : 4);
    }

    CANH_PushToTxQueue(instance, CANH_TxState1_16);
}
                            
void CANH_Send_TxVoltage1_4(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToTxQueue(instance, CANH_TxVoltage1_4);
}

void CANH_Send_TxVoltage5_8(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToTxQueue(instance, CANH_TxVoltage5_8);
}

void CANH_Send_TxVoltage9_12(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToTxQueue(instance, CANH_TxVoltage9_12);
}

void CANH_Send_TxVoltage13_16(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToTxQueue(instance, CANH_TxVoltage13_16);
}

void CANH_Send_TxCurrent1_4(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent1_4.data.current_4ch.current[0] = c1;
    CANH_TxCurrent1_4.data.current_4ch.current[1] = c2;
    CANH_TxCurrent1_4.data.current_4ch.current[2] = c3;
    CANH_TxCurrent1_4.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrent1_4);
}

void CANH_Send_TxCurrent5_8(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent5_8.data.current_4ch.current[0] = c1;
    CANH_TxCurrent5_8.data.current_4ch.current[1] = c2;
    CANH_TxCurrent5_8.data.current_4ch.current[2] = c3;
    CANH_TxCurrent5_8.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrent5_8);
}

void CANH_Send_TxCurrent9_12(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent9_12.data.current_4ch.current[0] = c1;
    CANH_TxCurrent9_12.data.current_4ch.current[1] = c2;
    CANH_TxCurrent9_12.data.current_4ch.current[2] = c3;
    CANH_TxCurrent9_12.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrent9_12);
}

void CANH_Send_TxCurrent13_16(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent13_16.data.current_4ch.current[0] = c1;
    CANH_TxCurrent13_16.data.current_4ch.current[1] = c2;
    CANH_TxCurrent13_16.data.current_4ch.current[2] = c3;
    CANH_TxCurrent13_16.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrent13_16);
}

void CANH_Send_SysStatus(T_CANH_INSTANCE instance, uint8_t sysStatus, uint16_t battVoltage, int16_t coreTemp, uint8_t safetyState, uint16_t logicValidMask)
{
    CANH_TxSysStatus.data.system_status.status = sysStatus;
    CANH_TxSysStatus.data.system_status.battVoltage = battVoltage;
    CANH_TxSysStatus.data.system_status.coreTemp = coreTemp;
    CANH_TxSysStatus.data.system_status.safetyLineState = safetyState;
    CANH_TxSysStatus.data.system_status.logicValidMask = logicValidMask;

    CANH_PushToTxQueue(instance, CANH_TxSysStatus);
}

void CANH_Send_Names(T_CANH_INSTANCE instance, uint8_t id, uint8_t part, char str[7], uint32_t fragSize)
{
    CANH_TxNames.data.raw[0] = (part << 4) + (id & 0x0F); 
    memset(&(CANH_TxNames.data.raw[1]), 0x00, 7);
    memcpy(&(CANH_TxNames.data.raw[1]), str, fragSize);

    CANH_PushToTxQueue(instance, CANH_TxNames);
}

void CANH_Send_PhyInputs1_4(T_CANH_INSTANCE instance, uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4)
{
    CANH_TxPhyInputs1_4.data.phy_inputs_4ch.input[0] = i1;
    CANH_TxPhyInputs1_4.data.phy_inputs_4ch.input[1] = i2;
    CANH_TxPhyInputs1_4.data.phy_inputs_4ch.input[2] = i3;
    CANH_TxPhyInputs1_4.data.phy_inputs_4ch.input[3] = i4;

    CANH_PushToTxQueue(instance, CANH_TxPhyInputs1_4);
}

void CANH_Send_PhyInputs5_8(T_CANH_INSTANCE instance, uint16_t i5, uint16_t i6, uint16_t i7, uint16_t i8)
{
    CANH_TxPhyInputs5_8.data.phy_inputs_4ch.input[0] = i5;
    CANH_TxPhyInputs5_8.data.phy_inputs_4ch.input[1] = i6;
    CANH_TxPhyInputs5_8.data.phy_inputs_4ch.input[2] = i7;
    CANH_TxPhyInputs5_8.data.phy_inputs_4ch.input[3] = i8;

    CANH_PushToTxQueue(instance, CANH_TxPhyInputs5_8);
}

void CANH_Send_ImuAcc(T_CANH_INSTANCE instance, int16_t accX, int16_t accY, int16_t accZ)
{
    CANH_TxImuAcc.data.imu_data.d1 = accX;
    CANH_TxImuAcc.data.imu_data.d2 = accY;
    CANH_TxImuAcc.data.imu_data.d3 = accZ;

    CANH_PushToTxQueue(instance, CANH_TxImuAcc);
}

void CANH_Send_ImuRates(T_CANH_INSTANCE instance, int16_t pitch, int16_t roll, int16_t yaw)
{
    CANH_TxImuGyro.data.imu_data.d1 = pitch;
    CANH_TxImuGyro.data.imu_data.d2 = roll;
    CANH_TxImuGyro.data.imu_data.d3 = yaw;

    CANH_PushToTxQueue(instance, CANH_TxImuGyro);
}

void CANH_Send_TxCurrentRMS1_4(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrentRMS1_4.data.current_4ch.current[0] = c1;
    CANH_TxCurrentRMS1_4.data.current_4ch.current[1] = c2;
    CANH_TxCurrentRMS1_4.data.current_4ch.current[2] = c3;
    CANH_TxCurrentRMS1_4.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrentRMS1_4);
}

void CANH_Send_TxCurrentRMS5_8(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrentRMS5_8.data.current_4ch.current[0] = c1;
    CANH_TxCurrentRMS5_8.data.current_4ch.current[1] = c2;
    CANH_TxCurrentRMS5_8.data.current_4ch.current[2] = c3;
    CANH_TxCurrentRMS5_8.data.current_4ch.current[3] = c4;

    CANH_PushToTxQueue(instance, CANH_TxCurrentRMS5_8);
}

void CANH_Send_I2tHeat1_8(T_CANH_INSTANCE instance, uint8_t h1, uint8_t h2, uint8_t h3, uint8_t h4, uint8_t h5, uint8_t h6, uint8_t h7, uint8_t h8)
{
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[0] = h1;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[1] = h2;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[2] = h3;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[3] = h4;

    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[4] = h5;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[5] = h6;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[6] = h7;
    CANH_TxI2tHeat1_8.data.i2t_heat_8ch.heat[7] = h8;

    CANH_PushToTxQueue(instance, CANH_TxI2tHeat1_8);
}

void CANH_Send_SocTreshold_1_4(T_CANH_INSTANCE instance, uint16_t oc1, uint16_t oc2, uint16_t oc3, uint16_t oc4)
{
    CANH_TxSOCThreshold1_4.data.current_4ch.current[0] = oc1;
    CANH_TxSOCThreshold1_4.data.current_4ch.current[1] = oc2;
    CANH_TxSOCThreshold1_4.data.current_4ch.current[2] = oc3;
    CANH_TxSOCThreshold1_4.data.current_4ch.current[3] = oc4;

    CANH_PushToTxQueue(instance, CANH_TxSOCThreshold1_4);
}

void CANH_Send_SocTreshold_5_8(T_CANH_INSTANCE instance, uint16_t oc1, uint16_t oc2, uint16_t oc3, uint16_t oc4)
{
    CANH_TxSOCThreshold5_8.data.current_4ch.current[0] = oc1;
    CANH_TxSOCThreshold5_8.data.current_4ch.current[1] = oc2;
    CANH_TxSOCThreshold5_8.data.current_4ch.current[2] = oc3;
    CANH_TxSOCThreshold5_8.data.current_4ch.current[3] = oc4;

    CANH_PushToTxQueue(instance, CANH_TxSOCThreshold5_8);
}

void CANH_Send_Hash(T_CANH_INSTANCE instance, uint32_t hash)
{
    CANH_TxHashResp.data.hash_resp.hash = hash;

    CANH_PushToTxQueue(instance, CANH_TxHashResp);
}

void CANH_Send_PWMDuty(T_CANH_INSTANCE instance, uint8_t ch1, uint8_t ch2, uint8_t ch3, uint8_t ch4, uint8_t ch5, uint8_t ch6, uint8_t ch7, uint8_t ch8)
{

    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[0] = ch1;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[1] = ch2;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[2] = ch3;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[3] = ch4;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[4] = ch5;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[5] = ch6;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[6] = ch7;
    CANH_TxPWMDuty.data.pwm_duty_8ch.duty[7] = ch8;

    CANH_PushToTxQueue(instance, CANH_TxPWMDuty);
}

void CANH_Send_DevDiag(T_CANH_INSTANCE instance, uint8_t sysLoad)
{
    CANH_TxDevDiag.data.dev_diag.sysLoad = sysLoad;

    CANH_PushToTxQueue(instance, CANH_TxDevDiag);
}

static void CANH_SwitchTerminator(T_CANH_INSTANCE instance, bool state)
{   
    if(instance == CANH_INSTANCE_1)
    {
        if(TRUE == state)
        {
            LL_GPIO_SetOutputPin(CAN_TERM1_GPIO_Port, CAN_TERM1_Pin);
        }
        else
        {
            LL_GPIO_ResetOutputPin(CAN_TERM1_GPIO_Port, CAN_TERM1_Pin);
        }
    }
    else if(instance == CANH_INSTANCE_2)
    {
        if(TRUE == state)
        {
            LL_GPIO_SetOutputPin(CAN_TERM2_GPIO_Port, CAN_TERM2_Pin);
        }
        else
        {
            LL_GPIO_ResetOutputPin(CAN_TERM2_GPIO_Port, CAN_TERM2_Pin);
        }
    }
}

void CANH_PushToTxQueue(T_CANH_INSTANCE instance, T_CANH_TX_PACKAGE pkg)
{
    if (cansCfg[instance].enabled == TRUE)
    {
        if (cansReg[instance].readyForTx == TRUE &&
            cansReg[instance].txQueueHandle != NULL &&
            xQueueSend(cansReg[instance].txQueueHandle, &pkg, portMAX_DELAY) != pdPASS)
        {
            /* Problem with pushing data to queue */
            LOG_WARN("CANH:: Unable to push can data to queue, queue full?");
            __NOP();
        }
        else if (cansReg[instance].txQueueHandle == NULL)
        {
            /* Queue handle does not exist yet */
            LOG_ERR("CANH:: CAN TX queue does not exist");
            __NOP();
        };
    }
}

void CANH_PushToRxQueue(T_CANH_INSTANCE instance, T_CANH_RX_PACKAGE pkg)
{
    if (cansCfg[instance].enabled == TRUE)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (cansReg[instance].readyForTx == TRUE &&
            cansReg[instance].rxQueueHandle != NULL &&
            xQueueSendFromISR(cansReg[instance].rxQueueHandle, &pkg, &xHigherPriorityTaskWoken) != pdPASS)
        {
            /* Problem with pushing data to queue */
            LOG_WARN("CANH:: Unable to push can data to RX queue, queue full?");
            __NOP();
        }
        else if (cansReg[instance].rxQueueHandle == NULL)
        {
            /* Queue handle does not exist yet */
            LOG_ERR("CANH:: CAN RX queue does not exist");
            __NOP();
        };
    }
}

static void CANH_InitModule(T_CANH_INSTANCE instance)
{
    // Set or unset can terminator analog switch based on configuration
    CANH_SwitchTerminator(instance, cansCfg[instance].terminator);

    /* Create queue for CAN TX data*/
    cansReg[instance].txQueueHandle = xQueueCreate(40, sizeof(T_CANH_TX_PACKAGE));
    cansReg[instance].rxQueueHandle = xQueueCreate(10, sizeof(T_CANH_RX_PACKAGE));

    // Initialize TX queue
    if(cansReg[instance].txQueueHandle == NULL)
    {
        /* Error creating xQueue -> heap too small [?] */
        LOG_ERR("Unable to create CAN TX queue");
        return;
    }

    // Initialize RX queue
    if(cansReg[instance].rxQueueHandle == NULL)
    {
        /* Error creating xQueue -> heap too small [?] */
        LOG_ERR("Unable to create CAN RX queue");
        return;
    }

    if(instance == CANH_INSTANCE_1)
    {
        CANH_InitFilterInstance1();
    }
    else if(instance == CANH_INSTANCE_2)
    {
        CANH_InitFilterInstance2();
    }

    cansReg[instance].isInited = TRUE;
}

static void CANH_AllowTxCallback1(void)
{
    /* Send CAN1 hello message */
    CANH_TxPnp.header.StdId = cansCfg[CANH_INSTANCE_1].baseId + CANH_ID_TX_PNP;
    HAL_CAN_AddTxMessage(&hcan1, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, cansReg[CANH_INSTANCE_1].txMailbox);
#ifdef DEBUG
    CANH_TxFwCommitHash.header.StdId = cansCfg[CANH_INSTANCE_1].baseId + CANH_ID_TX_FW_COMMIT_HASH;
    HAL_CAN_AddTxMessage(&hcan1, &(CANH_TxFwCommitHash.header), CANH_TxFwCommitHash.data.raw, cansReg[CANH_INSTANCE_1].txMailbox);
#endif
    cansReg[CANH_INSTANCE_1].readyForTx = TRUE;
}

static void CANH_AllowTxCallback2(void)
{
    /* Send CAN2 hello message */
    CANH_TxPnp.header.StdId = cansCfg[CANH_INSTANCE_2].baseId + CANH_ID_TX_PNP;
    HAL_CAN_AddTxMessage(&hcan2, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, cansReg[CANH_INSTANCE_2].txMailbox);
#ifdef DEBUG
    CANH_TxFwCommitHash.header.StdId = cansCfg[CANH_INSTANCE_2].baseId + CANH_ID_TX_FW_COMMIT_HASH;
    HAL_CAN_AddTxMessage(&hcan2, &(CANH_TxFwCommitHash.header), CANH_TxFwCommitHash.data.raw, cansReg[CANH_INSTANCE_2].txMailbox);
#endif
    cansReg[CANH_INSTANCE_2].readyForTx = TRUE;
}

static bool CANH_ConfigFilter4ID(T_CANH_INSTANCE instance, uint32_t filterBankNumber, uint16_t id1, uint16_t id2, uint16_t id3, uint16_t id4)
{
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filterBankNumber;                       
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; 
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    sFilterConfig.SlaveStartFilterBank = CANH_FILTER_INSTANCE2_FIRST_BANK;

    sFilterConfig.FilterIdHigh     = id1 << 5;
    sFilterConfig.FilterIdLow      = id2 << 5;         
    sFilterConfig.FilterMaskIdHigh = id3 << 5;
    sFilterConfig.FilterMaskIdLow  = id4 << 5;

    if(HAL_CAN_ConfigFilter(cansReg[instance].hcan, &sFilterConfig) != HAL_OK)
    {
        LOG_ERR("CANH:: Failed to configure CAN filter for ID list!");
        return FALSE;
    } 

    return TRUE;
}

/// @brief Initialize dynamic filter based on BSP CAN inputs
/// @param instance CAN instance for which the filter should be initialized
/// @param firstDynamicBankNumber First filter bank number that can be used for dynamic configuration (other banks are used for static filters)
/// @return TRUE if filter initialization was successful, FALSE if not enough filter banks for all inputs
static bool CANH_InitDynamicFilter(T_CANH_INSTANCE instance, uint32_t firstDynamicBankNumber)
{
    // Dynamic filter configuration for BSP inputs
    uint16_t filterPassIdsArr[4] = {0x7FF, 0x7FF, 0x7FF, 0x7FF};
    uint32_t filterPassIdsCount = 0;
    uint32_t filterBankNumber = firstDynamicBankNumber;
    const uint32_t lastUsedBankNumber = (instance == CANH_INSTANCE_1) ? CANH_FILTER_INSTANCE1_LAST_BANK : CANH_FILTER_INSTANCE2_LAST_BANK;

    for(uint32_t i = 0; i < IN_CAN_MAX; i++)
    {
        if(TRUE == BSP_CANIN_IsInputUsed(i))
        {
            T_BSP_CANIN_CFG* incfg = BSP_CANIN_GetInputCfg(i);

            if(incfg->canInstance == instance)
            {
                filterPassIdsArr[filterPassIdsCount] = incfg->canId;
                filterPassIdsCount++;

                if(filterPassIdsCount >= 4)
                {
                    if(filterBankNumber <= lastUsedBankNumber)
                    {
                        if(CANH_ConfigFilter4ID(instance, filterBankNumber, filterPassIdsArr[0], filterPassIdsArr[1], filterPassIdsArr[2], filterPassIdsArr[3]) != TRUE)
                        {
                            return FALSE;
                        } 
                        filterPassIdsCount = 0;
                        filterBankNumber++;

                        filterPassIdsArr[0] = 0x7FF;
                        filterPassIdsArr[1] = 0x7FF;
                        filterPassIdsArr[2] = 0x7FF;
                        filterPassIdsArr[3] = 0x7FF;
                    }
                    else
                    {
                        LOG_ERR("CANH:: Not enough filter banks for CAN BSP inputs! Some inputs will not pass the filter!");
                        return FALSE;
                    }
                }
            }
        }
    }

    if(filterPassIdsCount > 0 && filterBankNumber <= lastUsedBankNumber)
    {
        // Configure remaining filter IDs if there are any
        if(CANH_ConfigFilter4ID(instance, filterBankNumber, filterPassIdsArr[0], filterPassIdsArr[1], filterPassIdsArr[2], filterPassIdsArr[3]) != TRUE)
        {
            return FALSE;
        } 
    }

    return TRUE;
}

static bool CANH_InitFilterInstance1(void)
{
    bool result = TRUE;

    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = CANH_FILTER_INSTANCE1_FIRST_BANK;                       
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; 
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    sFilterConfig.SlaveStartFilterBank = CANH_FILTER_INSTANCE2_FIRST_BANK;

    // Pass force reset frame only
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;   

    // Matches EXACTLY 0x123
    sFilterConfig.FilterIdHigh     = (cansCfg[CANH_INSTANCE_2].baseId + CANH_ID_RX_FORCE_RESET_GW) << 5;
    sFilterConfig.FilterIdLow      = 0x7FF << 5;         
    sFilterConfig.FilterMaskIdHigh = 0x7FF << 5;
    sFilterConfig.FilterMaskIdLow  = 0x7FF << 5;

    result = (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) == HAL_OK);

    result &= CANH_InitDynamicFilter(CANH_INSTANCE_1, CANH_FILTER_INSTANCE1_FIRST_BANK + 1);

    return result;
}

static bool CANH_InitFilterInstance2(void)
{
    bool result = TRUE;

    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = CANH_FILTER_INSTANCE2_FIRST_BANK;                       
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; 
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    sFilterConfig.SlaveStartFilterBank = CANH_FILTER_INSTANCE2_FIRST_BANK;

    // Pass configuration frames (0x050 - 0x057) and force reset command (0x0AF)
    sFilterConfig.FilterIdHigh = (cansCfg[CANH_INSTANCE_2].baseId + CANH_ID_RX_ISOTP_ENTRANCE) << 5;             
    sFilterConfig.FilterMaskIdHigh = (0x7F8 << 5) | 0x08; 

    sFilterConfig.FilterIdLow = (cansCfg[CANH_INSTANCE_2].baseId + CANH_ID_RX_FORCE_RESET_GW) << 5;                  
    sFilterConfig.FilterMaskIdLow = (0x7FF << 5) | 0x08; 

    result = (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) == HAL_OK);

    result &= CANH_InitDynamicFilter(CANH_INSTANCE_2, CANH_FILTER_INSTANCE2_FIRST_BANK + 1);

    return result;
}

void CANH_Init(void)
{
    // Intialize CAN instances and TX queue
    if(TRUE == cansCfg[CANH_INSTANCE_1].enabled)
    {
        CANH_InitModule(CANH_INSTANCE_1);
    }

    if(TRUE == cansCfg[CANH_INSTANCE_2].enabled)
    {
        CANH_InitModule(CANH_INSTANCE_2);
    }
}


void CANH_ReconfigureAll(void)
{
    RTOS_SuspendCAN_1();
    RTOS_SuspendCAN_2();

    vPortEnterCritical();

    if(TRUE == cansCfg[CANH_INSTANCE_1].enabled)
    {
        HAL_CAN_Stop(&hcan1);
    }

    if(TRUE == cansCfg[CANH_INSTANCE_2].enabled)
    {
        HAL_CAN_Stop(&hcan2);
    }

    CANH_Init();

    vPortExitCritical();

    if(TRUE == cansCfg[CANH_INSTANCE_1].enabled)
    {
        HAL_CAN_Start(&hcan1);
        HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
        RTOS_ResumeCAN_1();
    }   

    if(TRUE == cansCfg[CANH_INSTANCE_2].enabled)
    {
        HAL_CAN_Start(&hcan2);
        HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
        RTOS_ResumeCAN_2();
    }
}


void can1TaskStart(void *argument)
{
    LOG_INFO("CAN1:: Task start");

    // canAllowAllFilter.FilterBank = 0;
    // if (HAL_CAN_ConfigFilter(&hcan1, &canAllowAllFilter) != HAL_OK)
    // {
    //     LOG_ERR("CANH:: CAN1 filter configuration failed!");
    // }

    if(cansCfg[CANH_INSTANCE_1].enabled == FALSE)
    {
        LOG_WARN("CANH:: CAN1 is disabled in configuration!");
        vTaskSuspend(NULL);
    }

    /* Start CAN1 -  CAN periph is started here so RX queue won't overfill if unexpected latency in init sequence happens */
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    osTimerId_t can1TxStartTimer = osTimerNew((osTimerFunc_t)CANH_AllowTxCallback1, osTimerOnce, NULL, NULL);
    if(can1TxStartTimer)
    {
        osTimerStart(can1TxStartTimer, pdMS_TO_TICKS(100));
    }
    else
    {
        LOG_ERR("CANH:: Unable to create CAN1 tx start timer!");
    }

    /* Incoming package form queue */
    T_CANH_TX_PACKAGE canPackage;

    for(;;)
    {
        /* Ongoing error on CAN1 */
        if(hcan1.State == HAL_CAN_STATE_ERROR)
        {
            __NOP();
        }
        
        if(cansReg[CANH_INSTANCE_1].readyForTx == TRUE && cansReg[CANH_INSTANCE_1].txQueueHandle != NULL 
            && xQueueReceive(cansReg[CANH_INSTANCE_1].txQueueHandle, &canPackage, pdMS_TO_TICKS(1)) == pdPASS)
        {
            CAN_TxHeaderTypeDef header = canPackage.header;
            header.StdId = canPackage.header.StdId + cansCfg[CANH_INSTANCE_1].baseId;
            HAL_CAN_AddTxMessage(&hcan1, &header, canPackage.data.raw, cansReg[CANH_INSTANCE_1].txMailbox);
        }

        if(cansReg[CANH_INSTANCE_1].rxQueueHandle != NULL)
        {
            T_CANH_RX_PACKAGE rxPkg;
            if(xQueueReceive(cansReg[CANH_INSTANCE_1].rxQueueHandle, &rxPkg, 0) == pdPASS)
            {
                /* Process received package */
                BSP_CANIN_DispatchFrame(CANH_INSTANCE_1, rxPkg.header.StdId, rxPkg.data.raw, rxPkg.header.DLC);

                // Force reset command reception
                if(rxPkg.header.StdId == CANH_ID_RX_FORCE_RESET_GW + cansCfg[CANH_INSTANCE_1].baseId)
                {
                    PDM_CANResetGateway(rxPkg.header.StdId, rxPkg.data.raw, rxPkg.header.DLC);
                }
            }
        }
        osDelay(1);
    }
}

void can2TaskStart(void *argument)
{
    LOG_INFO("CAN2:: Task start");

    // canAllowAllFilter.FilterBank = 14;
    // if (HAL_CAN_ConfigFilter(&hcan2, &canAllowAllFilter) != HAL_OK)
    // {
    //     LOG_ERR("CANH:: CAN2 filter configuration failed!");
    // }

    if(cansCfg[CANH_INSTANCE_2].enabled == FALSE)
    {
        LOG_WARN("CANH:: CAN2 is disabled in configuration!");
        vTaskSuspend(NULL);
    }

    /* Start CAN2  - CAN periph is started here so RX queue won't overfill if unexpected latency in init sequence happens*/
    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);

    osTimerId_t can2TxStartTimer = osTimerNew((osTimerFunc_t)CANH_AllowTxCallback2, osTimerOnce, NULL, NULL);
    if(can2TxStartTimer)
    {
        osTimerStart(can2TxStartTimer, pdMS_TO_TICKS(100));
    }
    else
    {
        LOG_ERR("CANH:: Unable to create CAN2 TX start timer!");
    }

    /* Incoming package from queue */
    T_CANH_TX_PACKAGE canPackage;

    for(;;)
    {
        /* Ongoing error on CAN2 */
        if(hcan2.State == HAL_CAN_STATE_ERROR)
        {
            __NOP();
        }
        
        if(cansReg[CANH_INSTANCE_2].readyForTx == TRUE && cansReg[CANH_INSTANCE_2].txQueueHandle != NULL 
            && xQueueReceive(cansReg[CANH_INSTANCE_2].txQueueHandle, &canPackage, pdMS_TO_TICKS(1)) == pdPASS)
        {
            CAN_TxHeaderTypeDef header = canPackage.header;
            header.StdId = canPackage.header.StdId + cansCfg[CANH_INSTANCE_2].baseId;
            HAL_CAN_AddTxMessage(&hcan2, &header, canPackage.data.raw, cansReg[CANH_INSTANCE_2].txMailbox);
        }
        
        if(cansReg[CANH_INSTANCE_2].rxQueueHandle != NULL)
        {
            T_CANH_RX_PACKAGE rxPkg;
            if(xQueueReceive(cansReg[CANH_INSTANCE_2].rxQueueHandle, &rxPkg, 0) == pdPASS)
            {
                /* Process received package */
                BSP_CANIN_DispatchFrame(CANH_INSTANCE_2, rxPkg.header.StdId, rxPkg.data.raw, rxPkg.header.DLC);
                
                // ISO-TP related frames (entrance GW and data frame reception) -- only CAN2
                if(rxPkg.header.StdId == CANH_ID_RX_ISOTP_ENTRANCE + cansCfg[CANH_INSTANCE_2].baseId)
                {
                    APP_ISOTP_CANEntranceGateway(rxPkg.header.StdId, rxPkg.data.raw, rxPkg.header.DLC);
                }
                else if(rxPkg.header.StdId == CANH_ID_RX_ISOTP + cansCfg[CANH_INSTANCE_2].baseId)
                {
                    APP_ISOTP_DispatchFrame(rxPkg.data.raw, rxPkg.header.DLC);
                }
                else if(rxPkg.header.StdId == CANH_ID_RX_CONFIG_REQ + cansCfg[CANH_INSTANCE_2].baseId)
                {
                    // Right now config A is hardcoded - B unused
                    uint32_t size = 0;
                    uint8_t* cfgPtr = NULL;
                    CONFIG_GetCfgExpSize(&size);
                    CONFIG_GetCfgPtr(CONFIG_SELECTION_A, &cfgPtr);
                    APP_ISOTP_Send(cfgPtr, size);
                }
                else if(rxPkg.header.StdId == CANH_ID_RX_HASH_REQ + cansCfg[CANH_INSTANCE_2].baseId)
                {
                    CANH_Send_Hash(CANH_INSTANCE_2, CONFIG_GetCurrentConfigCrc());
                }
                
                // Force reset command reception
                if(rxPkg.header.StdId == CANH_ID_RX_FORCE_RESET_GW + cansCfg[CANH_INSTANCE_2].baseId)
                {
                    PDM_CANResetGateway(rxPkg.header.StdId, rxPkg.data.raw, rxPkg.header.DLC);
                }
            }
        }

        osDelay(1);
    }
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    T_CANH_RX_PACKAGE rxPkg;

    // CAN1 MSG
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &(rxPkg.header), rxPkg.data.raw) == HAL_OK)
        {
            CANH_PushToRxQueue(CANH_INSTANCE_1, rxPkg);
        }
    }
    // CAN2 MSG
    else if (hcan->Instance == CAN2)
    {
        if (HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0, &(rxPkg.header), rxPkg.data.raw) == HAL_OK)
        {
            CANH_PushToRxQueue(CANH_INSTANCE_2, rxPkg);
        }
    }
}