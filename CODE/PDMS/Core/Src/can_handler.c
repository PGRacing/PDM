#include "can_handler.h"
#include "cmsis_os2.h"

xQueueHandle can1QueueHandle;
xQueueHandle can2QueueHandle;

/* CAN1 TxMailbox */
uint32_t can1TxMailbox[4];

/* CAN2 TxMailbox */
uint32_t can2TxMailbox[4];

/////////////////////////
//////// CAN TX /////////
/////////////////////////
/* All CAN TX structures should be added here*/

#define CANH_TX_DEFAULT_BYTE 0xAA

/// @brief All used CAN bus ID's
typedef enum
{
    CANH_ID_PNP           = 0x500, // Header sent on device power-up
    CANH_ID_SYS_STATUS    = 0x300, // System status
    CANH_ID_STATUS_1_8    = 0x301, // Status of channels 1-8 (default value)
    CANH_ID_STATUS_9_16   = 0x302, // Status of channels 9-16 (default value)
    CANH_ID_VOLTAGE_1_4   = 0x303, // Voltage of channels 1-4 (default value)
    CANH_ID_VOLTAGE_5_8   = 0x304, // Voltage of channels 5-8 (default value)
    CANH_ID_VOLTAGE_9_12  = 0x305, // Voltage of channels 9-12 (default value)
    CANH_ID_VOLTAGE_13_16 = 0x306, // Voltage of channels 13-16 (default value)
    CANH_ID_CURRENT_1_4   = 0x306, // Current of channels 1-4 (default value)
    CANH_ID_CURRENT_5_8   = 0x306, // Current of channels 5-8 (default value)
    CANH_ID_CURRENT_9_12  = 0x306, // Current of channels 9-12 (default value)
    CANH_ID_CURRENT_13_16 = 0x306, // Current of channels 13-16 (default value)
}T_CANH_ID;

/// @brief [pnpTxMsg] Message sent on device power-up
T_CANH_TX_PACKAGE CANH_TxPnp =
{
    .header = 
    {
        .DLC = 8,
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
    .data = {CANH_TX_DEFAULT_BYTE}
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
    .data = {CANH_TX_DEFAULT_BYTE}
};

void can1TaskStart(void *argument)
{
    /* Create queue for CAN1 data*/
    can1QueueHandle = xQueueCreate(5, sizeof(T_CANH_TX_PACKAGE));

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
    /* Create queue for CAN2 data */
    can2QueueHandle = xQueueCreate(5, sizeof(T_CANH_TX_PACKAGE));

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