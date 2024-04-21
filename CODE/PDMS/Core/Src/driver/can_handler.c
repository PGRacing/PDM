#include "can_handler.h"
#include "cmsis_os2.h"
#include "typedefs.h"

xQueueHandle can1QueueHandle;
xQueueHandle can2QueueHandle;

/* CAN1 TxMailbox */
uint32_t can1TxMailbox[4];

/* CAN2 TxMailbox */
uint32_t can2TxMailbox[4];

#define CANH_DEFAULT_TERM1_ENABLED 1
#define CANH_DEFAULT_TERM2_ENABLED 1

/////////////////////////
//////// CAN TX /////////
/////////////////////////
/* All CAN TX structures should be added here*/

#define CANH_TX_DEFAULT_BYTE 0xAA

/// @brief All used CAN bus ID's
typedef enum
{
    CANH_ID_PNP           = 0x400, // Header sent on device power-up
    CANH_ID_SYS_STATUS    = 0x401, // System status
    CANH_ID_STATUS_1_8    = 0x402, // Status of channels 1-8 (default value)
    CANH_ID_STATUS_9_16   = 0x403, // Status of channels 9-16 (default value)
    CANH_ID_VOLTAGE_1_4   = 0x404, // Voltage of channels 1-4 (default value)
    CANH_ID_VOLTAGE_5_8   = 0x405, // Voltage of channels 5-8 (default value)
    CANH_ID_VOLTAGE_9_12  = 0x406, // Voltage of channels 9-12 (default value)
    CANH_ID_VOLTAGE_13_16 = 0x407, // Voltage of channels 13-16 (default value)
    CANH_ID_CURRENT_1_4   = 0x408, // Current of channels 1-4 (default value)
    CANH_ID_CURRENT_5_8   = 0x409, // Current of channels 5-8 (default value)
    CANH_ID_CURRENT_9_12  = 0x410, // Current of channels 9-12 (default value)
    CANH_ID_CURRENT_13_16 = 0x411, // Current of channels 13-16 (default value)
}T_CANH_ID;

/// @brief [pnpTxMsg] Message sent on device power-up
T_CANH_TX_PACKAGE CANH_TxPnp =
{
    .header = 
    {
        .DLC = 1,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_PNP,
        .TransmitGlobalTime = DISABLE,
    },
    .data = {CANH_TX_DEFAULT_BYTE},
};

T_CANH_TX_PACKAGE CANH_TxStatus1_8 = 
{
    .header = 
    {
        .DLC = 8,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_STATUS_1_8,
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
        .StdId = CANH_ID_STATUS_9_16,
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
        .StdId = CANH_ID_VOLTAGE_1_4,
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
        .StdId = CANH_ID_VOLTAGE_5_8,
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
        .StdId = CANH_ID_VOLTAGE_9_12,
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
        .StdId = CANH_ID_VOLTAGE_13_16,
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
        .StdId = CANH_ID_CURRENT_1_4,
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
        .StdId = CANH_ID_CURRENT_5_8,
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
        .StdId = CANH_ID_CURRENT_9_12,
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
        .StdId = CANH_ID_CURRENT_13_16,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

void CANH_Send_TxStatus1_8(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8)
{   
    CANH_TxStatus1_8.data.status_8ch.status[0] = s1;
    CANH_TxStatus1_8.data.status_8ch.status[1] = s2;
    CANH_TxStatus1_8.data.status_8ch.status[2] = s3;
    CANH_TxStatus1_8.data.status_8ch.status[3] = s4;
    CANH_TxStatus1_8.data.status_8ch.status[4] = s5;
    CANH_TxStatus1_8.data.status_8ch.status[5] = s6;
    CANH_TxStatus1_8.data.status_8ch.status[6] = s7;
    CANH_TxStatus1_8.data.status_8ch.status[7] = s8;

    CANH_PushToQueue1(CANH_TxStatus9_16);
}

void CANH_Send_TxStatus9_16(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t s7, uint8_t s8)
{   
    CANH_TxStatus1_8.data.status_8ch.status[0] = s1;
    CANH_TxStatus1_8.data.status_8ch.status[1] = s2;
    CANH_TxStatus1_8.data.status_8ch.status[2] = s3;
    CANH_TxStatus1_8.data.status_8ch.status[3] = s4;
    CANH_TxStatus1_8.data.status_8ch.status[4] = s5;
    CANH_TxStatus1_8.data.status_8ch.status[5] = s6;
    CANH_TxStatus1_8.data.status_8ch.status[6] = s7;
    CANH_TxStatus1_8.data.status_8ch.status[7] = s8;

    CANH_PushToQueue1(CANH_TxStatus9_16);
}

void CANH_Send_TxVoltage1_4(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage1_4.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToQueue1(CANH_TxVoltage1_4);
}

void CANH_Send_TxVoltage5_8(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage5_8.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToQueue1(CANH_TxVoltage5_8);
}

void CANH_Send_TxVoltage9_12(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage9_12.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToQueue1(CANH_TxVoltage9_12);
}

void CANH_Send_TxVoltage13_16(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[0] = v1;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[1] = v2;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[2] = v3;
    CANH_TxVoltage13_16.data.voltage_4ch.voltage[3] = v4;

    CANH_PushToQueue1(CANH_TxVoltage13_16);
}

void CANH_Send_TxCurrent1_4(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent1_4.data.current_4ch.current[0] = c1;
    CANH_TxCurrent1_4.data.current_4ch.current[1] = c2;
    CANH_TxCurrent1_4.data.current_4ch.current[2] = c3;
    CANH_TxCurrent1_4.data.current_4ch.current[3] = c4;

    CANH_PushToQueue1(CANH_TxCurrent1_4);
}

void CANH_Send_TxCurrent5_8(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent5_8.data.current_4ch.current[0] = c1;
    CANH_TxCurrent5_8.data.current_4ch.current[1] = c2;
    CANH_TxCurrent5_8.data.current_4ch.current[2] = c3;
    CANH_TxCurrent5_8.data.current_4ch.current[3] = c4;

    CANH_PushToQueue1(CANH_TxCurrent5_8);
}

void CANH_Send_TxCurrent9_12(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent9_12.data.current_4ch.current[0] = c1;
    CANH_TxCurrent9_12.data.current_4ch.current[1] = c2;
    CANH_TxCurrent9_12.data.current_4ch.current[2] = c3;
    CANH_TxCurrent9_12.data.current_4ch.current[3] = c4;

    CANH_PushToQueue1(CANH_TxCurrent9_12);
}

void CANH_Send_TxCurrent13_16(uint16_t c1, uint16_t c2, uint16_t c3, uint16_t c4)
{
    CANH_TxCurrent13_16.data.current_4ch.current[0] = c1;
    CANH_TxCurrent13_16.data.current_4ch.current[1] = c2;
    CANH_TxCurrent13_16.data.current_4ch.current[2] = c3;
    CANH_TxCurrent13_16.data.current_4ch.current[3] = c4;

    CANH_PushToQueue1(CANH_TxCurrent13_16);
}

void CANH_SwitchTerminator1(bool state)
{   
    if(true == state)
    {
        LL_GPIO_SetOutputPin(CAN_TERM1_GPIO_Port, CAN_TERM1_Pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(CAN_TERM1_GPIO_Port, CAN_TERM1_Pin);
    }
}

void CANH_SwitchTerminator2(bool state)
{   
    if(true == state)
    {
        LL_GPIO_SetOutputPin(CAN_TERM2_GPIO_Port, CAN_TERM2_Pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(CAN_TERM2_GPIO_Port, CAN_TERM2_Pin);
    }
}

void CANH_PushToQueue1(T_CANH_TX_PACKAGE pkg)
{
    if(can1QueueHandle != NULL && xQueueSend(can1QueueHandle, &pkg, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }
    else if(can1QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}

void CANH_PushToQueue2(T_CANH_TX_PACKAGE pkg)
{
    if(can2QueueHandle != NULL && xQueueSend(can2QueueHandle, &pkg, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }
    else if(can2QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}

void can1TaskStart(void *argument)
{
    #ifdef CANH_DEFAULT_TERM1_ENABLED
        CANH_SwitchTerminator1(true);
    #else
        CANH_SwitchTerminator1(false);
    #endif
    /* Create queue for CAN1 data*/
    can1QueueHandle = xQueueCreate(20, sizeof(T_CANH_TX_PACKAGE));

    if(can1QueueHandle == NULL)
        /* Error creating xQueue -> heap too small [?] */
        __NOP();

    /* Start CAN1 */
    HAL_CAN_Start(&hcan1);

    /* Send CAN hello message */
    osDelay(pdMS_TO_TICKS(100));
    HAL_CAN_AddTxMessage(&hcan1, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, can1TxMailbox);

    /* Incoming package */
    T_CANH_TX_PACKAGE canPackage;

    for(;;)
    {
        /* Ongoing error on CAN1 */
        if(hcan1.State == HAL_CAN_STATE_ERROR)
            __NOP();

        if(can1QueueHandle != NULL && xQueueReceive(can1QueueHandle, &canPackage, portMAX_DELAY) == pdPASS)
        {
            HAL_CAN_AddTxMessage(&hcan1, &(canPackage.header), canPackage.data.raw, can1TxMailbox);
        }
        osDelay(1);
    }
}

void can2TaskStart(void *argument)
{
    #ifdef CANH_DEFAULT_TERM2_ENABLED
        CANH_SwitchTerminator2(true);
    #else
        CANH_SwitchTerminator2(false);
    #endif

    /* Create queue for CAN2 data */
    can2QueueHandle = xQueueCreate(20, sizeof(T_CANH_TX_PACKAGE));

    if(can2QueueHandle == NULL)
        /* Error creating xQueue -> heap too small [?] */
        __NOP();

    /* Start CAN2 */
    HAL_CAN_Start(&hcan2);

    /* Send CAN hello message */
    osDelay(pdMS_TO_TICKS(100));
    HAL_CAN_AddTxMessage(&hcan1, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, can1TxMailbox);

    /* Incoming package */
    T_CANH_TX_PACKAGE canPackage;

    for(;;)
    {
        /* Ongoing error on CAN2 */
        if(hcan2.State == HAL_CAN_STATE_ERROR)
            __NOP();

        if(can2QueueHandle != NULL && xQueueReceive(can2QueueHandle, &canPackage, portMAX_DELAY) == pdPASS)
        {
            HAL_CAN_AddTxMessage(&hcan2, &(canPackage.header), canPackage.data.raw, can2TxMailbox);
        }

        osDelay(1);
    }
}