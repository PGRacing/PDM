#include "can_handler.h"
#include "cmsis_os2.h"
#include "typedefs.h"
#include "logger.h"
#include "string.h"
#include "semphr.h"
#include "bsp_caninput.h"
#include "tim.h"

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

static CAN_FilterTypeDef canAllowAllFilter = 
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

/////////////////////////
//////// CAN TX /////////
/////////////////////////
/* All CAN TX structures should be added here*/

#define CANH_TX_DEFAULT_BYTE 0xAA

/// @brief All used CAN bus ID's
typedef enum
{
    CANH_ID_PNP           = 0x000, // Header sent on device power-up
    CANH_ID_SYS_STATUS    = 0x001, // System status
    CANH_ID_STATUS_1_8    = 0x002, // Status of channels 1-8 (default value)
    CANH_ID_STATUS_9_16   = 0x003, // Status of channels 9-16 (default value)
    CANH_ID_STATE_1_16    = 0x004, // State of channels 1-16 (default value)
    CANH_ID_VOLTAGE_1_4   = 0x005, // Voltage of channels 1-4 (default value)
    CANH_ID_VOLTAGE_5_8   = 0x006, // Voltage of channels 5-8 (default value)
    CANH_ID_VOLTAGE_9_12  = 0x007, // Voltage of channels 9-12 (default value)
    CANH_ID_VOLTAGE_13_16 = 0x008, // Voltage of channels 13-16 (default value)
    CANH_ID_CURRENT_1_4   = 0x009, // Current of channels 1-4 (default value)
    CANH_ID_CURRENT_5_8   = 0x00A, // Current of channels 5-8 (default value)
    CANH_ID_CURRENT_9_12  = 0x00B, // Current of channels 9-12 (default value)
    CANH_ID_CURRENT_13_16 = 0x00C, // Current of channels 13-16 (default value)
    CANH_ID_NAMES         = 0x00D, // Channel of current name
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

T_CANH_TX_PACKAGE CANH_TxSysStatus = 
{
    .header = 
    {
        .DLC = sizeof(T_CANH_SYSTEM_STATUS),
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .StdId = CANH_ID_SYS_STATUS,
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
        .StdId = CANH_ID_STATE_1_16,
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
        .StdId = CANH_ID_NAMES,
        .TransmitGlobalTime = DISABLE,
    },
    .data.raw = {CANH_TX_DEFAULT_BYTE}
};

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

void CANH_Send_SysStatus(T_CANH_INSTANCE instance, uint8_t sysStatus, uint16_t battVoltage, int16_t coreTemp, uint8_t safetyState)
{
    CANH_TxSysStatus.data.system_status.status = sysStatus;
    CANH_TxSysStatus.data.system_status.battVoltage = battVoltage;
    CANH_TxSysStatus.data.system_status.coreTemp = coreTemp;
    CANH_TxSysStatus.data.system_status.safetyLineState = safetyState;

    CANH_PushToTxQueue(instance, CANH_TxSysStatus);
}

void CANH_Send_Names(T_CANH_INSTANCE instance, uint8_t id, uint8_t part, char str[7])
{
    CANH_TxNames.data.raw[0] = (part << 4) + (id & 0x0F); 
    memcpy(&(CANH_TxNames.data.raw[1]), str, 7);

    CANH_PushToTxQueue(instance, CANH_TxNames);
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
    cansReg[instance].txQueueHandle = xQueueCreate(20, sizeof(T_CANH_TX_PACKAGE));
    cansReg[instance].rxQueueHandle = xQueueCreate(10, sizeof(T_CANH_RX_PACKAGE));

    // Initialize TX queue
    if(cansReg[instance].txQueueHandle == NULL)
    {
        /* Error creating xQueue -> heap too small [?] */
        LOG_ERR("Unable to create CAN TX queue");
        __NOP();
    }

    // Initialize RX queue
    if(cansReg[instance].rxQueueHandle == NULL)
    {
        /* Error creating xQueue -> heap too small [?] */
        LOG_ERR("Unable to create CAN RX queue");
        __NOP();
    }
}

static void CANH_AllowTxCallback1(void)
{
    /* Send CAN1 hello message */
    HAL_CAN_AddTxMessage(&hcan1, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, cansReg[CANH_INSTANCE_1].txMailbox);
    cansReg[CANH_INSTANCE_1].readyForTx = TRUE;
}

static void CANH_AllowTxCallback2(void)
{
    /* Send CAN2 hello message */
    HAL_CAN_AddTxMessage(&hcan2, &(CANH_TxPnp.header), CANH_TxPnp.data.raw, cansReg[CANH_INSTANCE_2].txMailbox);
    cansReg[CANH_INSTANCE_2].readyForTx = TRUE;
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

void can1TaskStart(void *argument)
{
    LOG_INFO("CAN1:: Task start");

    canAllowAllFilter.FilterBank = 0;
    if (HAL_CAN_ConfigFilter(&hcan1, &canAllowAllFilter) != HAL_OK)
    {
        LOG_ERR("CANH:: CAN1 filter configuration failed!");
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
            }
        }
        osDelay(1);
    }
}

void can2TaskStart(void *argument)
{
    LOG_INFO("CAN2:: Task start");

    canAllowAllFilter.FilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan2, &canAllowAllFilter) != HAL_OK)
    {
        LOG_ERR("CANH:: CAN2 filter configuration failed!");
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