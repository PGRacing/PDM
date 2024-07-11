#include "FreeRTOS.h"
#include "queue.h"
#include "can.h"
#include "stddef.h"

// CAN2.0B max paylaod size
#define CANH_MAX_DLC 8

/* CAN1 TxMailbox */
extern uint32_t can1TxMailbox[4];

/* CAN2 TxMailbox */
extern uint32_t can2TxMailbox[4];

/////////////////////////
//////// CAN TX /////////
/////////////////////////

typedef struct __packed _T_CANH_SYSTEM_STATUS
{
    uint8_t  status;
    uint16_t battVoltage;
    uint8_t  safetyLineState;
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

typedef union __packed
{
    uint8_t raw[8];
    T_CANH_SYSTEM_STATUS system_status;
    T_CANH_TX_STATUS_8CH status_8ch;
    T_CANH_TX_STATE_16CH state_16ch;
    T_CANH_TX_VOLTAGE_4CH voltage_4ch;
    T_CANH_TX_CURRENT_4CH current_4ch;
}T_CANH_DATA;

typedef struct __packed T_CANH_TX_PACKAGE 
{
    const CAN_TxHeaderTypeDef header;
    T_CANH_DATA data;
}T_CANH_TX_PACKAGE;

/* Can tasks */
void can1TaskStart(void *argument);
void can2TaskStart(void *argument);

void CANH_PushToQueue1(T_CANH_TX_PACKAGE pkg);
void CANH_PushToQueue2(T_CANH_TX_PACKAGE pkg);

extern xQueueHandle can1QueueHandle;
extern xQueueHandle can2QueueHandle;

/* TX MESSAGES */
void CANH_Send_TxVoltage1_4(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage5_8(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage9_12(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);
void CANH_Send_TxVoltage13_16(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4);

void CANH_Send_TxCurrent1_4(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent5_8(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent9_12(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);
void CANH_Send_TxCurrent13_16(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4);

void CANH_Send_TxStatus1_8(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8);
void CANH_Send_TxStatus9_16(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8);

void CANH_Send_TxState1_16(uint8_t s[16]);

void CANH_Send_SysStatus(uint8_t sysStatus, uint16_t battVoltage, uint8_t safetyLineStatus);

void CANH_Send_Names(uint8_t id, uint8_t part, char str[7]);