#ifndef __CAN_HANDLER_H_
#define __CAN_HANDLER_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "can.h"
#include "stddef.h"

// CAN2.0B max paylaod size
#define CANH_MAX_DLC 8

typedef enum
{
    CANH_INSTANCE_1    = 0x00, // First CAN BUS instance 
    CANH_INSTANCE_2    = 0x01, // Second CAN BUS instance
    CANH_INSTANCE_MAX  = 0x02, // Max number of CAN BUS instances
}T_CANH_INSTANCE;

typedef struct __packed
{
    bool     enabled; // Is can bus instance enabled>
    uint32_t baud; // Reserved fixed on 1Mbit/s right now
    bool     terminator; // CANBus terminator enabled?
    uint32_t baseId; // Base ID for this CAN instance (all frame ID will start from here)
}T_CANH_CFG;

typedef struct
{
    CAN_HandleTypeDef *hcan;
    QueueHandle_t txQueueHandle;
    QueueHandle_t rxQueueHandle;
    bool readyForTx;
    uint32_t txMailbox[4];
}T_CANH_REG;

/////////////////////////
//////// CAN TX /////////
/////////////////////////

typedef struct __packed _T_CANH_SYSTEM_STATUS
{
    uint8_t  status;
    uint16_t battVoltage;
    int16_t coreTemp;
    uint8_t  safetyLineState;
    uint16_t logicValidMask; // Bitmask of valid logic outputs, bit i is set if output i is valid
}T_CANH_SYSTEM_STATUS;

typedef struct __packed _T_CANH_TX_STATUS_8CH
{
   uint8_t status[8]; // Propably will be changed to some kind of casted enum
}T_CANH_TX_STATUS_8CH; 

typedef struct __packed _T_CANH_TX_STATE_16CH
{
   uint8_t state[8];
}T_CANH_TX_STATE_16CH; 

typedef struct __packed _T_CANH_TX_VOLTAGE_4CH
{
    uint16_t voltage[4]; // Voltage of channels passed in mV value (0 - 65535 mV)
}T_CANH_TX_VOLTAGE_4CH;

typedef struct __packed _T_CANH_TX_CURRENT_4CH
{
    uint16_t current[4]; // Current passed in 10e-2 amps [centi ampers] for eg. 100cA - 1A (0 - 65535 cA / 0 - 655.35 A)
}T_CANH_TX_CURRENT_4CH;

typedef struct  __packed _T_CANH_PHY_INPUTS_4CH
{
    uint16_t input[4]; // Physical inputs passed in mV value (0 - 5000 mV)
}T_CANH_PHY_INPUTS_4CH;

typedef struct __packed _T_CANH_IMU_DATA
{
    int16_t d1;
    int16_t d2;
    int16_t d3;
}T_CANH_IMU_DATA;

typedef union __packed
{
    uint8_t raw[8];
    T_CANH_SYSTEM_STATUS system_status;
    T_CANH_TX_STATUS_8CH status_8ch;
    T_CANH_TX_STATE_16CH state_16ch;
    T_CANH_TX_VOLTAGE_4CH voltage_4ch;
    T_CANH_TX_CURRENT_4CH current_4ch;
    T_CANH_PHY_INPUTS_4CH phy_inputs_4ch;
    T_CANH_IMU_DATA imu_data;
}T_CANH_DATA;

typedef struct T_CANH_TX_PACKAGE 
{
    CAN_TxHeaderTypeDef header;
    T_CANH_DATA data;
}T_CANH_TX_PACKAGE;

typedef struct T_CANH_RX_PACKAGE 
{
    CAN_RxHeaderTypeDef header;
    T_CANH_DATA data;
}T_CANH_RX_PACKAGE;


/// @brief Main CAN configuration
extern T_CANH_CFG cansCfg[CANH_INSTANCE_MAX];

/* CAN tasks */
void can1TaskStart(void *argument);
void can2TaskStart(void *argument);

/// @brief CAN bus module initialization
void CANH_Init(void);

void CANH_PushToTxQueue(T_CANH_INSTANCE instance, T_CANH_TX_PACKAGE pkg);
void CANH_PushToRxQueue(T_CANH_INSTANCE instance, T_CANH_RX_PACKAGE pkg);

/* TX MESSAGES */
void CANH_Send_TxVoltage1_4(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage5_8(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage9_12(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage13_16(T_CANH_INSTANCE instance, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);

void CANH_Send_TxCurrent1_4(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent5_8(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent9_12(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent13_16(T_CANH_INSTANCE instance, uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);

void CANH_Send_TxStatus1_8(T_CANH_INSTANCE instance, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8);
void CANH_Send_TxStatus9_16(T_CANH_INSTANCE instance, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8);

void CANH_Send_TxState1_16(T_CANH_INSTANCE instance, uint8_t s[16]);

void CANH_Send_SysStatus(T_CANH_INSTANCE instance, uint8_t sysStatus, uint16_t battVoltage, int16_t coreTemp, uint8_t safetyLineStatus, uint16_t logicValidMask);

void CANH_Send_Names(T_CANH_INSTANCE instance, uint8_t id, uint8_t part, char str[7], uint32_t fragSize);

void CANH_Send_PhyInputs1_4(T_CANH_INSTANCE instance, uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4);
void CANH_Send_PhyInputs5_8(T_CANH_INSTANCE instance, uint16_t i5, uint16_t i6, uint16_t i7, uint16_t i8);

void CANH_Send_ImuAcc(T_CANH_INSTANCE instance, int16_t accX, int16_t accY, int16_t accZ);
void CANH_Send_ImuRates(T_CANH_INSTANCE instance, int16_t pitch, int16_t roll, int16_t yaw);

#endif