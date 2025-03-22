#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_it.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "tim.h"

/* HAL, MX includes */
#include "iwdg.h"

/* Modules includes */
#include "typedefs.h"
#include "features.h"
#include "logger.h"
#include "logic.h"
#include "out.h"
#include "vmux.h"
#include "adc_handler.h"
#include "can_handler.h"
#include "telemetry.h"
#include "buzzer.h"
#include "ws2812b.h"
#include "pdm.h"
// Main PDM file

#define UVLO_TIMER_PERIOD 50

extern void RTOS_SoftLimpHomeMode(void);

T_PDM_SYS_STATUS PDM_GetSysStatus(void)
{
    // TODO Add status handlers CAN status etc...
    return PDM_SYS_STATUS_OK;
}

volatile T_PDM_CFG pdmCfg =
{
    .onInitBattCheckMaxTries = 50,
    .isUvloEnabled = true,
    .uvloVoltageHiThreshold = 10000, // 10V
    .uvloVoltageLoThreshold = 8000,  // 8V
    .uvloTimeThreshold = 10000, // 10s
    .uvloRetainDivider = 10, 
    .minBattVolage = 6000, // In BT50010LUA TDS 5.8V minimal voltage declared
    .canCfg = 
    {
        .can1Baud = 1000000,
        .can2Baud = 1000000,
        .can1Terminator = FALSE,
        .can2Terminator = TRUE,
    },
    .useBuzzer = TRUE,
    .ledMode = 1
};


volatile T_PDM_REG pdmReg =
{
    .status = PDM_SYS_STATUS_OK,
    .safetyState = true,
    .uvloAssessment = false,
    .uvloLoCounter = 0,
    .uvloHiCounter = 0,
    .uvloTimer = NULL
};

static T_OUT_MODE PDM_OutModeInitTable[OUT_ID_MAX] = 
{
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
    OUT_MODE_STD,
};

static void PDM_OutConfig(void)
{
    for(uint8_t i = 0; i < OUT_ID_MAX; i++)
    {
        OUT_ChangeMode( i, PDM_OutModeInitTable[i] );
    }
}

static void PDM_SoftLimpHomeModeTimerCallback()
{
    VMUX_ReadBattVoltage();
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);
    BUZZER_TurnOn();
    osDelay(1000);
    BUZZER_TurnOff();
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
}

// Soft limp home mode with active telemetry (can bus)
static void PDM_SoftLimpHomeMode(T_PDM_SYS_STATUS status)
{
    pdmReg.status = status;
    for(uint8_t id = 0; id < OUT_ID_MAX; id++)
    {
        OUT_SetState(id, OUT_STATE_ERR_LATCH);
    }

    // Check battery voltage manually each 30 seconds
    pdmReg.uvloTimer = osTimerNew((osTimerFunc_t)PDM_SoftLimpHomeModeTimerCallback, osTimerPeriodic, NULL, NULL);
    if(pdmReg.uvloTimer)
    {
        osTimerStart(pdmReg.uvloTimer, pdMS_TO_TICKS(30000));
    }
    else
    {
        LOG_ERR("PDM:: Unable to create UVLO timer!");
    }
    
    // Disable all leds
    WS2812B_DisableAll();

    // Disable all tasks except can handlers
    RTOS_SoftLimpHomeMode();
}

// Platform start
void PDM_Init(void)
{    
    vPortEnterCritical();
    
    /* ===== CONFIG LOAD ===== */
    // TODO Load settings from flash to respective flash structs(rn mockup)
    // TODO Add status return function for all initializers;

    /* ===== PERIPHERAL INIT ===== */

    // Initialize ADC acquisition handler
    ADCH_Init();

    // Initialize CAN communication handling 
    CANH_Init();

    // Initialize voltage measurement multiplexer
    VMUX_Init();

    /* ===== MODULES INITALIZATION ===== */
    // Initialize output module (set correct mode and state)
    PDM_OutConfig();

    // Iniitialize SPOC2 External outputs (all outputs disabled)
    SPOC2_Init();

    // Enable debug DWT clock
    DEBUG_ARM_DWT_INIT;

    /* ===== ON INIT CHECK ===== */
    
    // Wait some time for on-board capacitors to load
    uint32_t backoffDelay = 10000000; 
    while(backoffDelay--)
    {
        __NOP();
    }

    // Verify that voltage is sufficent for normal operation (repetetive turn-on protection)
    VMUX_ReadBattVoltage();
    uint32_t lastBattVoltage = VMUX_GetBattValue();
    
    for(uint32_t i = 0; i < pdmCfg.onInitBattCheckMaxTries; i++)
    {
        if(lastBattVoltage <= pdmCfg.minBattVolage)
        {
            backoffDelay = 10000000; 
            while(backoffDelay--)
            {
                __NOP();
            }

            LOG_WARN("PDM:: Insufficent battery voltage to start");
            LOG_VAR(lastBattVoltage);

            if(i == pdmCfg.onInitBattCheckMaxTries - 1)
            {
                LOG_ERR("PDM:: Unable to start battery voltage too low!");
                Error_Handler();
            }
        }
        else
        {
            // If battery voltage is sufficent to start exit loop
            break;
        }
    }

    LOG_INFO("PDM:: Board initialized successfully");
    LOG_VAR(lastBattVoltage);

    // Enable watchdog
    MX_IWDG_Init();
    vPortExitCritical();
}

// Get safety-line state
static void PDM_CheckSafety(void)
{
    if(HAL_GPIO_ReadPin(SAFETY_IN_GPIO_Port, SAFETY_IN_Pin) == GPIO_PIN_RESET)
    {

        pdmReg.safetyState = false;
    }
    else
    {
        pdmReg.safetyState = true;
    }
}

static void PDM_UVLOCallback()
{
    if(VMUX_GetBattValue() > pdmCfg.uvloVoltageHiThreshold)
    {
        pdmReg.uvloHiCounter++;
        if(pdmReg.uvloHiCounter > (pdmCfg.uvloTimeThreshold / (UVLO_TIMER_PERIOD * pdmCfg.uvloRetainDivider)))
        {
            osTimerStop(pdmReg.uvloTimer);
            pdmReg.uvloTimer = NULL;
            pdmReg.uvloAssessment = false;
            pdmReg.uvloHiCounter = 0;
            pdmReg.uvloLoCounter = 0;
            LOG_INFO("PDM:: UVLO Battery voltage retained, returing to normal operation");
            return;
        }
    }
    else
    {
        pdmReg.uvloLoCounter++;
        if(pdmReg.uvloLoCounter > (pdmCfg.uvloTimeThreshold / UVLO_TIMER_PERIOD))
        {
            osTimerStop(pdmReg.uvloTimer);
            pdmReg.uvloTimer = NULL;
            pdmReg.uvloAssessment = false;
            pdmReg.uvloHiCounter = 0;
            pdmReg.uvloLoCounter = 0;
            LOG_ERR("PDM:: UVLO detected turning off!");
            PDM_SoftLimpHomeMode(PDM_SYS_STATUS_UVLO);
            return;
        }
    }

}

// Check under voltage protection
static void PDM_CheckUVLO(void)
{
    // Trigger checking on timer
    if(VMUX_GetBattValue() < pdmCfg.uvloVoltageLoThreshold)
    {
        LOG_WARN("PDM:: First UVLO trigger detected");
        pdmReg.uvloAssessment = true;
        // Create timer for retry execution routine
        pdmReg.uvloTimer = osTimerNew((osTimerFunc_t)PDM_UVLOCallback, osTimerPeriodic, NULL, NULL);
        if(pdmReg.uvloTimer)
        {
            osTimerStart(pdmReg.uvloTimer, pdMS_TO_TICKS(UVLO_TIMER_PERIOD));
        }
        else
        {
            LOG_ERR("PDM:: Unable to create UVLO timer!");
        }
    }
}

bool PDM_GetSafetyState(void)
{
    return pdmReg.safetyState;
}

void pdmTaskStart(void *argument)
{       
    /* ---> Platform initialized (PDM_Init) in main.c before scheduler start */
    LOG_INFO("PDM:: Task start");
    for(;;)
    {
        bool* logicResults = LOGIC_Evaluate();
        for(uint8_t i = 0; i < OUT_ID_MAX; i++)
        {
            OUT_SetState(i, logicResults[i]);
        }
        PDM_CheckSafety();

        // Check undervoltage if no ongoing assesment
        if(pdmCfg.isUvloEnabled == true &&
            pdmReg.uvloAssessment == false &&
            pdmReg.status == PDM_SYS_STATUS_OK)
        {
            PDM_CheckUVLO();
        }
      
        osDelay(pdMS_TO_TICKS(1));
    }
}
