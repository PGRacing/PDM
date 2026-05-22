#include "config.h"
#include "buzzer.h"
#include "out.h"
#include "bsp_flash.h"
#include "string.h"
#include "logger.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

T_OUT_CFG (*configPtrA)[16] = (T_OUT_CFG (*)[16])0x080E0000; // Pointing to CONFIGB1 section in flash
T_OUT_CFG (*configPtrB)[16] = (T_OUT_CFG (*)[16])0x080F0000; // Pointing to CONFIGB1 section in flash

#define CONFIGB1_SECTOR 0 
#define CONFIGB2_SECTOR 32

#define CONFIG_EXPECTED_SIZE 1360

extern void RTOS_SuspendForConfigChange(void);
extern void RTOS_ResumeAfterConfigChange(void);

/// @brief Perform required action before applying new configuration (e.g. suspend tasks, set outputs to safe state etc.)
/// @param 
static void CONFIG_PreConfigChange(void)
{
    RTOS_SuspendForConfigChange();
    for(T_OUT_ID id = 0; id < OUT_ID_MAX; id++)
    {
        OUT_SetState(id, OUT_STATE_OFF);
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
    }
    else if(configSelection == CONFIG_SELECTION_B)
    {
        memcpy(outsCfg, configPtrB, sizeof(outsCfg));
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
        if(BSP_FLASH_Erease(CONFIGB1_SECTOR) == 0)
        {
            if(BSP_FLASH_Prog(CONFIGB1_SECTOR, 0, data, size) == 0)
            {
                return true;
            }
        }

    }
    else if(configSelection == CONFIG_SELECTION_B)
    {  
        if(BSP_FLASH_Erease(CONFIGB2_SECTOR) == 0)
        {
            if(BSP_FLASH_Prog(CONFIGB2_SECTOR, 0, data, size) == 0)
            {
                return true;
            }
        }
     }
    return false;
}

/// @brief Perform full sequence of applying new configuration (pre-change actions, config flush, post-change actions)
/// @param configSelection Selected configuration [A/B]
/// @param data Configuration data
/// @param size Size of configuration data
void CONFIG_NewConfig(T_CONFIG_SELECTION configSelection, uint8_t* data, uint32_t size)
{
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