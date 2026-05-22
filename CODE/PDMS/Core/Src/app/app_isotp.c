#include "app_isotp.h"
#include "isotp.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "logger.h"
#include "task.h"
#include "semphr.h"
#include "config.h"

SemaphoreHandle_t isotpAllowedSemaphore = NULL;

static const uint8_t isotpGateway[3][8] = {{0xfd, 0xf8, 0x0a, 0xf9, 0x66, 0x56, 0x18, 0x78}, {0x80, 0x5a, 0xef, 0xf0, 0x01, 0xf0, 0x70, 0x48}, {0x52, 0xa8, 0xb7, 0x20, 0xad, 0xad, 0xff, 0xf0}};
static uint8_t isotpGatewayCounter = 0;

#define DFU_RX_BUFSIZE 4096
#define DFU_TX_BUFSIZE 4096

volatile uint32_t debugCounter = 0;

typedef struct _T_ISOTP_CONFIG
{
    uint32_t isoTpEntrance;
    uint32_t isoTpTxId;
    uint32_t isoTpRxId;
}
T_ISOTP_CONFIG;

typedef struct _T_ISOTP_HANDLE
{
    bool isInit;
    // bool inRx;
    // uint32_t expRxSize; // Expected RX size
    // uint32_t rxSize;    // Received size so far
    // uint32_t blockSize; // Block size
    // uint32_t numberOfBlocks; //  Number of blocks
    // uint32_t expNumberOfBlocks; // Expected number of blocks
    IsoTpLink isoTpLink;
    uint8_t isotpRecvBuf[DFU_RX_BUFSIZE];
    uint8_t isotpSendBuf[DFU_TX_BUFSIZE];
    uint8_t flashBuffer[DFU_RX_BUFSIZE];
    // uint8_t image[DFU_FLASH_BUFFER_SIZE];
    // bool isotpDownDone;
    //bool flashProgDone;
}
T_ISOTP_HANDLE;

T_ISOTP_HANDLE isotpHandle = {0};

// Should reflect in CANH configuration as base ID for correct CAN instance
T_ISOTP_CONFIG isotpConfig = {
    .isoTpEntrance = 0x450, // entrance ID
    .isoTpTxId = 0x451, // TX ID 
    .isoTpRxId = 0x452  // Rx ID
};

extern void RTOS_SuspendTelemetry(void);
extern void RTOS_ResumeTelemetry(void);

/// @brief Initialize ISOTP module (pre RTOS start)
/// @param  
void APP_ISOTP_Init(void)
{
    isotpAllowedSemaphore = xSemaphoreCreateBinary();
}

/// @brief Entrace gateway for ISOTP
/// @param id CAN ID of received frame
/// @param data Data of received frame
/// @param size Size of received frame
/// @note This function should be called sequentially with frames defined in isotpGateway array, if sequence is correct then ISOTP task will be allowed to start and receive configuration data, otherwise sequence will be reset and ISOTP task will not start
void APP_ISOTP_EntranceGateway(uint32_t id, uint8_t* data, uint32_t size)
{
    if(size != 8)
    {
        isotpGatewayCounter = 0;
        return;
    }
    else if(size == 8 && 0 == memcmp(data, isotpGateway[isotpGatewayCounter], 8))
    {
        isotpGatewayCounter++;
        if(isotpGatewayCounter >= 3)
        {
            isotpGatewayCounter = 0;
            xSemaphoreGive(isotpAllowedSemaphore);
        }
    }
    else
    {
        isotpGatewayCounter = 0;
    }
}

/// @brief Start ISOTP session
/// @param handle ISO-TP handle
/// @param config ISO-TP configuration
static void APP_ISOTP_Start(T_ISOTP_HANDLE* handle, const T_ISOTP_CONFIG* config)
{
    isotp_init_link(&handle->isoTpLink, config->isoTpTxId,
                    handle->isotpSendBuf, sizeof(handle->isotpSendBuf), 
                    handle->isotpRecvBuf, sizeof(handle->isotpRecvBuf));
}

/// @brief Rx ready callback, called when full ISOTP message is received
/// @param handle ISO-TP handle
/// @param size Size of received message
static void APP_ISOTP_RxReady(T_ISOTP_HANDLE* handle, uint32_t size)
{
    CONFIG_NewConfig(CONFIG_SELECTION_A, handle->isotpRecvBuf, size);
}

/// @brief Dispatch received CAN frame to ISOTP module
/// @param data Data of received frame
/// @param dlc Data length of received frame
void APP_ISOTP_DispatchFrame(uint8_t*data, uint32_t dlc)
{
    isotp_on_can_message(&isotpHandle.isoTpLink, data, dlc);
    debugCounter++;
}

void isotpTaskStart(void *argument)
{
    LOG_INFO("ISOTP:: Task start");

    for (;;)
    {
        if(xSemaphoreTake(isotpAllowedSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Suspend telemetry task to avoid conflicts
            RTOS_SuspendTelemetry();
            APP_ISOTP_Start(&isotpHandle, &isotpConfig);
            for(;;)
            {
                isotp_poll(&isotpHandle.isoTpLink);
                uint32_t outSize = 0;

                int32_t ret = isotp_receive(&isotpHandle.isoTpLink, isotpHandle.flashBuffer, sizeof(isotpHandle.flashBuffer), &outSize);
                if (ISOTP_RET_OK == ret) {
                    APP_ISOTP_RxReady(&isotpHandle, outSize);
                    RTOS_ResumeTelemetry();
                    break;
                }
                else if(ISOTP_RET_NO_DATA != ret)
                {
                    // Break on error
                    RTOS_ResumeTelemetry();
                    break;
                }
                osDelay(pdMS_TO_TICKS(1));
            }
        }
        osDelay(pdMS_TO_TICKS(1));
    }
}