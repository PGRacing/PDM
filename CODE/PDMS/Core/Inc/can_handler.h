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

typedef struct _T_CANH_SYSTEM_STATUS
{
    uint8_t status;
}T_CANH_SYSTEM_STATUS;

typedef struct _T_CANH_TX_STATUS_8CH
{
   uint8_t status[8]; // Propably will be changed to some kind of casted enum
}T_CANH_TX_STATUS_8CH; 

typedef struct _T_CANH_TX_VOLTAGE_4CH
{
    uint16_t voltage[4]; // Voltage of channels passed in mV value (0 - 65535 mV)
}T_CANH_TX_VOLTAGE_4CH;

typedef struct _T_CANH_TX_CURRENT_4CH
{
    uint16_t current[4]; // Current passed in 10e-2 amps [centi ampers] for eg. 100cA - 1A (0 - 65535 cA / 0 - 655.35 A)
}T_CANH_TX_CURRENT_4CH;

typedef union __packed
{
    uint8_t raw[8];
    T_CANH_SYSTEM_STATUS system_status;
    T_CANH_TX_STATUS_8CH status_8ch;
    T_CANH_TX_VOLTAGE_4CH voltage_4ch;
    T_CANH_TX_CURRENT_4CH current_4ch;
}T_CANH_DATA;

typedef struct T_CANH_TX_PACKAGE
{
    CAN_TxHeaderTypeDef header;
    T_CANH_DATA data;
}T_CANH_TX_PACKAGE;

/* Can tasks */
void can1TaskStart(void *argument);
void can2TaskStart(void *argument);

extern xQueueHandle can1QueueHandle;
extern xQueueHandle can2QueueHandle;