#include "config.h"
#include "crc.h"
#include "buzzer.h"
#include "out.h"
#include "bsp_flash.h"
#include "string.h"
#include "logger.h"
#include "bsp_caninput.h"
#include "input.h"
#include "logic.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

T_OUT_CFG (*configPtrA)[16] = (T_OUT_CFG (*)[16])0x080E0000; // Pointing to CONFIGB1 section in flash
T_OUT_CFG (*configPtrB)[16] = (T_OUT_CFG (*)[16])0x080F0000; // Pointing to CONFIGB1 section in flash

#define CONFIGB1_START_SECTOR 0 
#define CONFIGB2_START_SECTOR 32

#define CONFIG_OUTPUT_SIZE sizeof(outsCfg) // OUTPUT configuration size in bytes (size of outsCfg array - PACKED)
#define CONFIG_BSP_CAN_SIZE sizeof(bspCanInputsCfg) // BSP CAN INPUT configuration size in bytes (size of bspCanInputsCfg array - PACKED)
#define CONFIG_INPUT_SIZE sizeof(inputsCfg) // INPUT configuration size in bytes (size of inputsCfg array - PACKED)
#define CONFIG_LOGIC_SIZE sizeof(logicCfg) // LOGIC configuration size in bytes (size of logicCfg array - PACKED)
#define CONFIG_CRC_SIZE sizeof(uint32_t) // CRC size in bytes

//                         [OUTPUT][BSP_CAN][INPUT][LOGIC][CRC]
//#define CONFIG_EXPECTED_SIZE 1360 + 144 + 96 + 192       + 4
#define CONFIG_EXPECTED_SIZE (CONFIG_OUTPUT_SIZE + CONFIG_BSP_CAN_SIZE + CONFIG_INPUT_SIZE + CONFIG_LOGIC_SIZE + CONFIG_CRC_SIZE)

#define CONFIG_EXPECTED_SIZE_IN_SECTOR DIV_CEIL(CONFIG_EXPECTED_SIZE, FLASH_SECTOR_SIZE)

#define CONFIG_OUTCFG_OFFSET  0
#define CONFIG_CANCFG_OFFSET  CONFIG_OUTPUT_SIZE
#define CONFIG_INPUTCFG_OFFSET (CONFIG_CANCFG_OFFSET + CONFIG_BSP_CAN_SIZE)
#define CONFIG_LOGICCFG_OFFSET (CONFIG_INPUTCFG_OFFSET + CONFIG_INPUT_SIZE)
#define CONFIG_CRC_OFFSET (CONFIG_LOGICCFG_OFFSET + CONFIG_LOGIC_SIZE)

extern void RTOS_SuspendForConfigChange(void);
extern void RTOS_ResumeAfterConfigChange(void);

uint32_t currentConfigCrc = 0x00000000;

/// @brief Calculate CRC32 of given data
/// @param data Ptr to data
/// @param size Size of data
/// @return CRC value for given data
static uint32_t CONFIG_CalcCRC(uint8_t* data, uint32_t size)
{
    return HAL_CRC_Calculate(&hcrc, (uint32_t*)data, size);
}

/// @brief Perform required action before applying new configuration (e.g. suspend tasks, set outputs to safe state etc.)
/// @param 
static void CONFIG_PreConfigChange(void)
{
    RTOS_SuspendForConfigChange();
    for(T_OUT_ID id = 0; id < OUT_ID_MAX; id++)
    {
        OUT_SetState(id, OUT_STATE_OFF);
        BSP_OUT_SetMode(id, OUT_MODE_UNUSED);
    }
    OUT_ResetRegistersAll();
}

/// @brief Load new configuration to the system
/// @param configSelection Selected configuration [A/B] 
void CONFIG_LoadConfig(T_CONFIG_SELECTION configSelection)
{
    if(configSelection == CONFIG_SELECTION_A)
    {
        memcpy(outsCfg, configPtrA, sizeof(outsCfg));
        memcpy(bspCanInputsCfg, (T_BSP_CANIN_CFG*)((uint8_t*)configPtrA + CONFIG_CANCFG_OFFSET), sizeof(bspCanInputsCfg));
        memcpy(inputsCfg, (T_IN_CFG*)((uint8_t*)configPtrA + CONFIG_INPUTCFG_OFFSET), sizeof(inputsCfg));
        memcpy(logicCfg, (T_LOGIC_CFG*)((uint8_t*)configPtrA + CONFIG_LOGICCFG_OFFSET), sizeof(logicCfg));
        currentConfigCrc = (uint32_t)(*(uint32_t*)((uint8_t*)configPtrA + CONFIG_CRC_OFFSET));
    }
    else if(configSelection == CONFIG_SELECTION_B)
    {
        memcpy(outsCfg, configPtrB, sizeof(outsCfg));
        memcpy(bspCanInputsCfg, (T_BSP_CANIN_CFG*)((uint8_t*)configPtrB + CONFIG_CANCFG_OFFSET), sizeof(bspCanInputsCfg));
        memcpy(inputsCfg, (T_IN_CFG*)((uint8_t*)configPtrB + CONFIG_INPUTCFG_OFFSET), sizeof(inputsCfg));
        memcpy(logicCfg, (T_LOGIC_CFG*)((uint8_t*)configPtrB + CONFIG_LOGICCFG_OFFSET), sizeof(logicCfg));
        currentConfigCrc = (uint32_t)(*(uint32_t*)((uint8_t*)configPtrB + CONFIG_CRC_OFFSET));
    }
}

/// @brief Apply new configuration to the system
/// @param configSelection Selected configuration [A/B] 
static void CONFIG_ApplyNewConfig(T_CONFIG_SELECTION configSelection)
{
    CONFIG_LoadConfig(configSelection);
    BUZZER_TurnOn();
    osDelay(100);
    BUZZER_TurnOff();
    osDelay(100);
    BUZZER_TurnOn();
    osDelay(100);
    BUZZER_TurnOff();
}

/// @brief Perform required action after applying new configuration (e.g. resume tasks, apply new settings to peripherals etc.)
/// @param 
static void CONFIG_PostConfigChange(void)
{
    // Reconfigure all channels
    OUT_ReconfigureAll();

    // Reconfigure CAN handlers (filters, callbacks etc.) based on new config
    CANH_ReconfigureAll();

    RTOS_ResumeAfterConfigChange();
}

/// @brief Flush new configuration to flash memory
/// @param configSelection Selected configuration [A/B]
/// @param data Configuration data
/// @param size Size of configuration data
/// @return true if successful, false otherwise
static bool CONFIG_FlushConfig(T_CONFIG_SELECTION configSelection, uint8_t* data, uint32_t size)
{
    if(size != CONFIG_EXPECTED_SIZE)
    {
        LOG_ERR("CONFIG:: Invalid config size!");
        return false;
    }

    if(configSelection == CONFIG_SELECTION_A)
    {
        for(uint32_t i = 0; i < CONFIG_EXPECTED_SIZE_IN_SECTOR; i++)
        {
            if(BSP_FLASH_Erease(CONFIGB1_START_SECTOR + i) != 0)
            {
                LOG_ERR("CONFIG:: Failed to erase flash sector!");
                return false;
            }
        }

         if(BSP_FLASH_Prog(CONFIGB1_START_SECTOR, 0, data, size) != 0)
         {
             LOG_ERR("CONFIG:: Failed to program flash!");
             return false;
         }
    }
    else if(configSelection == CONFIG_SELECTION_B)
    {  
        for(uint32_t i = 0; i < CONFIG_EXPECTED_SIZE_IN_SECTOR; i++)
        {
            if(BSP_FLASH_Erease(CONFIGB2_START_SECTOR + i) != 0)
            {
                LOG_ERR("CONFIG:: Failed to erase flash sector!");
                return false;
            }
        }

        if(BSP_FLASH_Prog(CONFIGB2_START_SECTOR, 0, data, size) != 0)
        {
            LOG_ERR("CONFIG:: Failed to program flash!");
            return false;
        }
    }
    return true;
}

static bool CONFIG_VerifyConfig(T_CONFIG_SELECTION configSelection, uint8_t* data, uint32_t size, uint32_t* crcOut)
{
    if(size != CONFIG_EXPECTED_SIZE)
    {
        LOG_ERR("CONFIG:: Invalid config size for CRC verification!");
        return false;
    }

    uint32_t expectedCrc = 0;
    memcpy(&expectedCrc, data + CONFIG_CRC_OFFSET, sizeof(uint32_t));
    *crcOut = CONFIG_CalcCRC(data, size - CONFIG_CRC_SIZE);

    if(expectedCrc != *crcOut)
    {
        LOG_ERR("CONFIG:: Invalid CRC");
        return false;
    }
    return true;
}

/// @brief Perform full sequence of applying new configuration (pre-change actions, config flush, post-change actions)
/// @param configSelection Selected configuration [A/B]
/// @param data Configuration data
/// @param size Size of configuration data
void CONFIG_NewConfig(T_CONFIG_SELECTION configSelection, uint8_t* data, uint32_t size)
{
    uint32_t calcCrc = 0x00000000;
    if(FALSE == CONFIG_VerifyConfig(configSelection, data, size, &calcCrc))
    {
        LOG_ERR("CONFIG:: New configuration verification failed! Aborting config change.");
        return;
    }

    if(CONFIG_FlushConfig(configSelection, data, size))
    {
        CONFIG_PreConfigChange();
        CONFIG_ApplyNewConfig(configSelection);
        CONFIG_PostConfigChange();
        LOG_INFO("CONFIG:: New config applied successfully!");
    }
    else
    {
        LOG_ERR("CONFIG:: Failed to flush new config to flash!");
    }
}

void CONFIG_GetCfgPtr(T_CONFIG_SELECTION configSelection, uint8_t** outPtr)
{
    if(configSelection == CONFIG_SELECTION_A)
    {
        *outPtr = (uint8_t*)configPtrA;
    }
    else if(configSelection == CONFIG_SELECTION_B)
    {
        *outPtr = (uint8_t*)configPtrB;
    }
}

void CONFIG_GetCfgExpSize(uint32_t* outSize)
{
    *outSize = CONFIG_EXPECTED_SIZE;
}

uint32_t CONFIG_GetCurrentConfigCrc(void)
{
    return currentConfigCrc;
}