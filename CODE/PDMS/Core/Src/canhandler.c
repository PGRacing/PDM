#include "canhandler.h"
#include "cmsis_os2.h"

xQueueHandle can1QueueHandle;
xQueueHandle can2QueueHandle;

/* CAN1 TxMailbox */
uint32_t can1TxMailbox[4];

/* CAN2 TxMailbox */
uint32_t can2TxMailbox[4];

/* Welcome message */
/* TODO add code stamp to pnpHeader */
CAN_TxHeaderTypeDef pnpCANHeader =
        {
                .DLC = 8,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x500,
                .TransmitGlobalTime = DISABLE,
        };

uint8_t pnpCANData[8] = {0xAA};


void can1TaskStart(void *argument)
{
    /* Create queue for CAN1 data*/
    can1QueueHandle = xQueueCreate(5, sizeof(CAN_TxPackageType));

    if(can1QueueHandle == NULL)
        /* Error creating xQueue -> heap too small [?] */
        __NOP();

    /* Start CAN1 */
    HAL_CAN_Start(&hcan1);

    /* Send CAN hello message */
    osDelay(pdMS_TO_TICKS(100));
    HAL_CAN_AddTxMessage(&hcan1, &(pnpCANHeader), pnpCANData, can1TxMailbox);

    /* Incoming package */
    CAN_TxPackageType canPackage;

    for(;;)
    {
        /* Ongoing error on CAN1 */
        if(hcan1.State == HAL_CAN_STATE_ERROR)
            __NOP();

        if(can1QueueHandle != NULL && xQueueReceive(can1QueueHandle, &canPackage, portMAX_DELAY) == pdPASS)
        {
            HAL_CAN_AddTxMessage(&hcan1, &(canPackage.header), canPackage.data, can1TxMailbox);
        }

        osDelay(1);
    }
}

void can2TaskStart(void *argument)
{
    /* Create queue for CAN2 data */
    can2QueueHandle = xQueueCreate(5, sizeof(CAN_TxPackageType));

    if(can2QueueHandle == NULL)
        /* Error creating xQueue -> heap too small [?] */
        __NOP();

    /* Start CAN2 */
    HAL_CAN_Start(&hcan2);

    /* Send CAN hello message */
    osDelay(pdMS_TO_TICKS(100));
    HAL_CAN_AddTxMessage(&hcan2, &(pnpCANHeader), pnpCANData, can2TxMailbox);

    /* Incoming package */
    CAN_TxPackageType canPackage;

    for(;;)
    {
        /* Ongoing error on CAN2 */
        if(hcan2.State == HAL_CAN_STATE_ERROR)
            __NOP();

        if(can2QueueHandle != NULL && xQueueReceive(can2QueueHandle, &canPackage, portMAX_DELAY) == pdPASS)
        {
            HAL_CAN_AddTxMessage(&hcan2, &(canPackage.header), canPackage.data, can2TxMailbox);
        }

        osDelay(1);
    }
}