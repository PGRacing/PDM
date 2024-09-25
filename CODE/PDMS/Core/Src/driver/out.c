#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "out.h"
#include "vmux.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "stdlib.h"
#include "cmsis_os2.h"
#include "ws2812b.h"

/// USER DEFINES
// Safety related defines
#define OUT_DIAG_READ_FREQ 200 // 200 Hz frequency of adc read
#define OUT_DIAG_READ_PERIOD (1000 / OUT_DIAG_READ_FREQ)
#define OUT_SAFETY_OC_OFF 0xFFFF
#define OUT_DIAG_MS_TO_OC_TRIP(X) ((X) / OUT_DIAG_READ_PERIOD)
#define OUT_DIAG_BTS_OPEN_LOAD_TRESHOLD 100 // [mA] Under this value output channel current is considered open load
#define OUT_DIAG_BTS_DETECT_OPEN_LOAD TRUE // Is open load detection enabled on BTS channels?
#define OUT_DIAG_BTS_VBAT_HISTERESIS 1000 // [mV] Acceptable variation of voltage on output to Vbatt
//#define OUT_DIAG_BTS500_ADC_VOLTAGE_TO_MA

/// FUNCTION PROTOTYPES
static T_OUT_CFG* OUT_GetCfgPtr( T_OUT_ID id );
static T_OUT_REG* OUT_GetRegPtr( T_OUT_ID id );

/// MACRO FUNCTIONS
#define OUT_ASSERT_IN_RANGE(id)      (ASSERT( (id) >= 0 && (id) < OUT_ID_MAX))
#define OUT_ASSERT_IN_RANGE_CFG(id)  (ASSERT( (id) >= 0 && (id) < ARRAY_COUNT(outsCfg)))
#define OUT_ASSERT_IN_RANGE_REG(id)  (ASSERT( (id) >= 0 && (id) < ARRAY_COUNT(outsReg)))

// Get current time in ms
#define OUT_GET_TIME_MS (pdTICKS_TO_MS( xTaskGetTickCount() ))

// do + while(0) here for correct macro evaluation
#define OUT_SAFETY_CALLBACK_BODY(id)   do{osTimerStop(OUT_GetRegPtr((id))->safety.timerHandle); \
                                  OUT_GetRegPtr((id))->safety.timerHandle = NULL; \
                                  OUT_SetState((id), OUT_STATE_ON); \
                                  OUT_GetRegPtr((id))->safety.inError = FALSE; }while(0)

#pragma region SAFETY_CALLBACKS

void OUT_CH1_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_1);
}
void OUT_CH2_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_2);
}
void OUT_CH3_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_3);
}
void OUT_CH4_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_4);
}
void OUT_CH5_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_5);
}
void OUT_CH6_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_6);
}
void OUT_CH7_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_7);
}
void OUT_CH8_SafetyCallback(void)
{
  OUT_SAFETY_CALLBACK_BODY(OUT_ID_8);
}

#pragma endregion

/// @brief Main output channels config [1..16]
T_OUT_CFG outsCfg[OUT_ID_MAX] =
{
        [OUT_ID_1] = {
            .id = OUT_ID_1,
            .name = "SERVO",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_TRY_RETRY,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 10000, // mA
              .socTripThreshold = 400, // 4 x ms
              .errRetryThreshold = 3, // 
              .timerInterval = 1000, 
              .safetyCallback = &OUT_CH1_SafetyCallback,
            }
        },
        [OUT_ID_2] = {
            .id = OUT_ID_2,
            .name = "CAN L",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .useSoc = TRUE,
              .socThreshold = 2000, // 2000mA Threshold
              .socTripThreshold = 0,
              .safetyCallback = &OUT_CH2_SafetyCallback,
            }
        },
        [OUT_ID_3] = {
            .id = OUT_ID_3,
            .name = "GCU",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_TRY_RETRY,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 8000, // mA
              .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 3, // 
              .timerInterval = 1000,
              .safetyCallback = &OUT_CH3_SafetyCallback,
            }
        },
        [OUT_ID_4] = {
            .id = OUT_ID_4,
            .name = "FAN1",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_LATCH,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 20000, // mA
              .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 3, // 
              .timerInterval = 1000,
              .safetyCallback = &OUT_CH4_SafetyCallback,
            }
        },
        [OUT_ID_5] = {
            .id = OUT_ID_5,
            .name = "MAINS",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_LATCH,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 12000, // mA
              .socTripThreshold = 4000, // 4 x ms
              .errRetryThreshold = 6, // 
              .timerInterval = 1000,
              .safetyCallback = &OUT_CH5_SafetyCallback,
            }
        },
        [OUT_ID_6] = {
            .id = OUT_ID_6,
            .name = "PUMP",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_LATCH,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 10000, // mA
              .socTripThreshold = 2600, // 4 x ms
              .errRetryThreshold = 6, // 
              .timerInterval = 1000,
              .safetyCallback = &OUT_CH6_SafetyCallback,
            }
        },
        [OUT_ID_7] = {
            .id = OUT_ID_7,
            .name = "FAN2",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .aerrCfg = OUT_ERR_BEH_LATCH,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 20000, // mA
              .socTripThreshold = 1600, // 4 x ms
              .errRetryThreshold = 3, // 
              .timerInterval = 1000,
              .safetyCallback = &OUT_CH7_SafetyCallback,
            }
        },
        [OUT_ID_8] = {
            .id = OUT_ID_8,
            .name = "ECU",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = {
              .aerrCfg = OUT_ERR_BEH_TRY_RETRY,
              .actOnSafety = FALSE,
              .useSoc = TRUE,
              .socThreshold = 5000, // mA
              .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 6, // 
              .timerInterval = 2000,
              .safetyCallback = &OUT_CH8_SafetyCallback,
            }
        },
        [OUT_ID_9] = {
            .id = OUT_ID_9,
            .name = "TELE B",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_10] = {
            .id = OUT_ID_10,
            .name = "E.LOG",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_11] = {
            .id = OUT_ID_11,
            .name = "TELE F",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_12] = {
            .id = OUT_ID_12,
            .name = "DASH",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_13] = {
            .id = OUT_ID_13,
            .name = "COMM",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_14] = {
            .id = OUT_ID_14,
            .name = "C.LOG",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_15] = {
            .id = OUT_ID_15,
            .name = "ADD 1",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_16] = {
            .id = OUT_ID_16,
            .name = "ADD 2",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },        

};

/// @brief Main output channel status register
T_OUT_REG outsReg[OUT_ID_MAX] =
{
  [OUT_ID_1] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_2] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_3] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_4] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_5] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_6] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_7] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_8] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_9] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_10] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_11] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_12] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_13] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_14] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_15] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_16] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .ocTripCounter = 0,
      .errRetryCounter = 0,
      .timerHandle = NULL,
      .inError = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  }
};

// Acquire config struct
#define OUT_GETCFGPTR(id) (&(outsCfg[(id)]))

/// @brief Acquire configuration struct with range assert
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Pointer to configuration struct
static T_OUT_CFG* OUT_GetCfgPtr( T_OUT_ID id )
{
  OUT_ASSERT_IN_RANGE_CFG(id);
  return &(outsCfg[id]);
}

/// Acquire status register of selected channel
#define OUT_GETREGPTR(id) (&(outsReg[(id)]))

/// @brief Acquire status register of selected channel with range assert
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Pointer to status register
static T_OUT_REG* OUT_GetRegPtr( T_OUT_ID id )
{
  OUT_ASSERT_IN_RANGE_REG(id);
  return &(outsReg[id]);
}

void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode)
{
  T_OUT_CFG* cfg = OUT_GetCfgPtr(id);
  ASSERT( cfg );

  switch (targetMode)
  {
  case OUT_MODE_UNUSED:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
       if (cfg->mode == OUT_MODE_PWM)
      {
        BSP_OUT_DeInitPWM(id);
      }
      BSP_OUT_SetMode(id, targetMode);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      // TODO Add handler
      //SPOC2_SetMode(id, targetMode);
    }
    cfg->mode = targetMode;
    break;
  }
  case OUT_MODE_STD:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      if (cfg->mode == OUT_MODE_PWM)
      {
        BSP_OUT_DeInitPWM(id);
      }
      BSP_OUT_SetMode(id, targetMode);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      // TODO Add handler
      // SPOC2_SetMode(id, targetMode);
    }
    OUT_SetState(cfg->id, OUT_STATE_OFF);
    cfg->mode = targetMode;
    break;
  }
  case OUT_MODE_PWM:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      BSP_OUT_SetMode(id, targetMode);
      // Set PWM duty to 0%
      BSP_OUT_SetDutyPWM(id, 0);
      cfg->mode = targetMode;
    }
    break;
  }
  case OUT_MODE_BATCH:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      LOG_WARN("Batching two inputs!");

      /* Batch shouldn't be out of range */
      ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));
      T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

      // LL MODE STD
      BSP_OUT_SetMode(id, OUT_MODE_STD);
      BSP_OUT_SetMode(cfg->batch, OUT_MODE_STD);
      OUT_SetState(cfg->id, OUT_STATE_OFF);
      OUT_SetState(batchCfg->id, OUT_STATE_OFF);
      batchCfg->mode = OUT_MODE_BATCH;
      /* From now on actions on batch outputs are performed simultaniously */
      cfg->mode = targetMode;
    }
    break;
  }

  default:
    break;
  }

  // TODO Should it be like this if we change state before??
  OUT_GetRegPtr(id)->state = OUT_STATE_OFF;
}

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE reqState)
{
  bool res = TRUE;

  OUT_ASSERT_IN_RANGE(id);

  // Acquire output channel config
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  // Acquire output channel status reg
  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  // If channel is latched it cannot be used
  if(reg->state == OUT_STATE_ERR_LATCH)
  {
    return FALSE;
  }

  // If channel in error handler it cannot be turned on
  if(reg->safety.inError == TRUE 
    && reqState == OUT_STATE_ON)
  {
    return FALSE;
  }

  // New state will be error latch (perform channel shutdown and block)
  if(reqState == OUT_STATE_ERR_LATCH)
  {
    OUT_SetState(id, OUT_STATE_OFF);
    reg->state = OUT_STATE_ERR_LATCH;
    return FALSE;
  }

  switch (cfg->mode)
  {
  case OUT_MODE_UNUSED:
  {
    LOG_WARN("Unable to set state on unused output config");
    res = FALSE;
    break;
  }
  case OUT_MODE_STD:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      // TODO This executes each time logic is evaluted change it!!!
      BSP_OUT_SetStdState(id, reqState);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      // If new state is applied
      if(reqState != reg->state)
      {
        SPOC2_SetStdState(cfg->spocId, cfg->spocChId, reqState);
      }
    }
    
    reg->state = reqState;
    res = TRUE;
    break;
  }
  case OUT_MODE_PWM:
  { 
    // TODO Allow on/off also on PWM channels
    LOG_WARN("Trying to set steady state on PWM output");
    res = FALSE;
    break;
  }
  case OUT_MODE_BATCH:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));

      T_OUT_REG* batchReg = OUT_GETREGPTR(cfg->batch);

      // Use this function to change GPIO register simultaniously for both switches
      BSP_OUT_SetBatchState(id, cfg->batch, reqState);

      reg->state = reqState;
      batchReg->state = reqState;
      res = TRUE;
    }
    else
    {
      res = FALSE;
    }

    break;
  }
  default:
    break;
  }

  return res;
}

bool OUT_Batch(T_OUT_ID id, T_OUT_ID batchId)
{
  /// TODO Add handling of SPOC2 internal batch function
  T_OUT_CFG* cfg = OUT_GetCfgPtr(id);
  T_OUT_CFG* batchCfg = OUT_GetCfgPtr(batchId);
  ASSERT(cfg);
  ASSERT(batchCfg);

  if(cfg->type != OUT_TYPE_BTS500 || batchCfg->type != OUT_TYPE_BTS500)
  {
    LOG_ERR("Batch working only on BTS500");
    return FALSE;
  }

  if (batchId >= ARRAY_COUNT(outsCfg))
  {
    LOG_ERR("Setting batch out of range!");
    return FALSE;
  }

  if(!BSP_OUT_IsBatchPossible(id, batchId))
  {
    LOG_ERR("Batch with diffrent port");
    return FALSE;
  }

  cfg->batch = batchId;
  batchCfg->batch = id;

  OUT_ChangeMode(id, OUT_MODE_BATCH);
  return TRUE;
}

bool OUT_ToggleState(T_OUT_ID id)
{
  T_OUT_REG* reg = OUT_GetRegPtr(id);
  ASSERT(reg);
  return OUT_SetState( id, !reg->state);
}

#pragma region DIAG_SECTION

/// @brief If TRY_RETRY is used this function will take care of retry routine
/// @param id Output channel id [1..16] T_OUT_ID
static void OUT_DIAG_DispatchRetry(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);

  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  // TODO This comparsion does not execute proper amount of times
  if(reg->safety.errRetryCounter >= cfg->safety.errRetryThreshold)
  {
    // Last error handle 
    if(reg->safety.timerHandle != NULL)
    {
      osTimerStop(reg->safety.timerHandle);
      reg->safety.timerHandle = NULL;
    }
    // Disable timer and latch output channel
    OUT_SetState(id, OUT_STATE_ERR_LATCH);
    reg->safety.inError = TRUE;

    return;
  }
  else if( reg->safety.timerHandle == NULL)
  {
    // Check if safety function callback does exist
    ASSERT(cfg->safety.safetyCallback);

    // Create timer for retry execution routine
    reg->safety.timerHandle = osTimerNew((osTimerFunc_t)cfg->safety.safetyCallback, osTimerOnce, NULL, NULL);
    if(reg->safety.timerHandle)
    {
      osTimerStart(reg->safety.timerHandle, pdMS_TO_TICKS(cfg->safety.timerInterval));
    }else
    {
      // If timer creation failed go straight to latch 
      OUT_SetState(id, OUT_STATE_ERR_LATCH);
    }
    
    reg->safety.inError = TRUE;
  }

  reg->safety.errRetryCounter++;
}

/// @brief Callback executed if switch turned to error, execute proper error handling routine
/// @param id Output channel id [1..16] T_OUT_ID
static void OUT_DIAG_OnErrorFallback(T_OUT_ID id)
{

  OUT_ASSERT_IN_RANGE(id);
  const T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  switch (cfg->safety.aerrCfg.behavior)
  {
  case OUT_ERR_BEH_NO:
    break;

  case OUT_ERR_BEH_LATCH:
    // Latch channel in off state till device reset
    OUT_SetState(id, OUT_STATE_ERR_LATCH);
    break; 

  case OUT_ERR_BEH_TRY_RETRY:
    OUT_SetState(id, OUT_STATE_OFF);
    // TODO This does not work deterministically (executes more than should)
    // TODO Shouldn't be based on osTimer, perform hardware timer implementation
    OUT_DIAG_DispatchRetry(id);
    break;

  default:
    break;
  }
}

static T_OUT_STATUS OUT_DIAG_SingleBtsHardware(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);

  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  T_OUT_STATUS newStatus = OUT_STATUS_OK;

  // Acquire needeed data
  reg->voltageMV = VMUX_GetValue(id);   // TODO Consider ADC data coherenece
  reg->currentMA = BSP_OUT_CalcCurrent(id);  
  uint32_t diffChVbat = abs((int32_t)(VMUX_GetBattValue() - reg->voltageMV));
  bool inFault = reg->currentMA > BSP_OUT_GetFaultLevel(id);
  bool noLoad  = reg->currentMA < OUT_DIAG_BTS_OPEN_LOAD_TRESHOLD;

  if(OUT_STATE_ON == reg->state)
  {
    if(diffChVbat < OUT_DIAG_BTS_VBAT_HISTERESIS)
    {
      if(OUT_DIAG_BTS_DETECT_OPEN_LOAD && TRUE == noLoad)
      {
        newStatus = OUT_STATUS_OPEN_LOAD;
      }
      else
      {
        newStatus = OUT_STATUS_OK;
      }
     
    }
    else
    {
      if(TRUE == inFault)
      {
        newStatus = OUT_STATUS_HARD_OC_OR_OT; 
      }
      else
      {
        newStatus = OUT_STATUS_CONTROL_FAIL;
      }
    }
  }
  else if(OUT_STATE_OFF == reg->state)
  {
    if(diffChVbat < OUT_DIAG_BTS_VBAT_HISTERESIS)
    {
      newStatus = OUT_STATUS_SHORT_TO_VSS;
    }
    else
    {
      newStatus = OUT_STATUS_OK;
    }
  }

  return newStatus;
}

/// @brief Perform all needed processing for output channel of BTS type
/// @param id Output channel id [1..8] T_OUT_ID
static void OUT_DIAG_SingleBts(T_OUT_ID id)
{
  // TODO Perform major rework based on comments and notion document
  OUT_ASSERT_IN_RANGE(id);
  // TODO First fix state machine
  
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  T_OUT_STATUS newStatus = OUT_STATUS_OK;

  // BTS500 ONLY
  // TODO There should be only one return do it elsewhere
  if( outsCfg[id].type != OUT_TYPE_BTS500)
  {
    return;
  }

  /// SOFT OC BLOCK START
  // TODO Access of current should be propably in critical section or some sync should be used, write to memory by DMA, when reading?
  // Not a real concern in case of in ISR execution on ADC
  // What if ADC readout is wrong or ADC is malfunctioning? Assure ADC safety
  // We do soft overcurrent block but we don't now current switch state (maybe hard oc), should be changed
  reg->currentMA = BSP_OUT_CalcCurrent(id);  

  if(TRUE == cfg->safety.useSoc)
  {
    if(reg->currentMA > cfg->safety.socThreshold)
    {
      if(reg->safety.ocTripCounter >= cfg->safety.socTripThreshold)
      {
        newStatus = OUT_STATUS_SOFT_OC;
        // TODO Make imidiate action on oc detection!!!
        reg->safety.ocTripCounter = 0;
      }else
      {
        if( OUT_STATE_ON == reg->state)
        {
          // TODO Shouldn't be increased by const implement some fusing current
          reg->safety.ocTripCounter += 4;
        }
      }
    }
    else
    {
      if(reg->safety.ocTripCounter > 0)
      {
        reg->safety.ocTripCounter--;
      }
    }
  }
  /// SOFT OC BLOCK END

  // TODO Implement I2t and heat protection block
  
  /// STATE DETECTION BLOCK START
  // TODO How is voltage synced with current adc readout???
  // What in case of ADC error or VMUX malfunction
  // Add better filtering
  reg->voltageMV = VMUX_GetValue(id);
  //uint32_t fault_level = BSP_OUT_GetDkilis(id) * 4;
  uint32_t batteryVoltage = VMUX_GetBattValue();
  // TODO This fault level should be changed 
  // TODO If new zenner diodes installed it should be variable due to used dkilis and resistor
  // TODO Change for better state detection
  uint32_t faultLevel = 12000;
  // TODO Remove not needed call just to access const memory location (performance issue)
  uint32_t dkilis = BSP_OUT_GetDkilis(id);
  // TODO Changed this value experimentally 
  uint32_t voltageHis = 1000; // 1000 mV for now

  T_OUT_STATUS hwStatus = OUT_STATUS_OK;
  if(OUT_STATE_OFF == reg->state)
  {
      if(reg->currentMA >= faultLevel)
      { 
        if(abs((int32_t)(batteryVoltage - reg->voltageMV) < voltageHis))
        {
          hwStatus = OUT_STATUS_SHORT_TO_VSS;
        }
        else
        {
          hwStatus = OUT_STATUS_OK;
        }
      }
  }
  else
  {
    // TODO Fix magic values
      if(reg->currentMA >= faultLevel)
      {
        // TODO WARN
        // No latch on HW
        //hwStatus = OUT_STATUS_HARD_OC_OR_OT;
      }
      // This is shit some in ampers some in mili amps
      else if(((reg->currentMA <= 0.0000143 * (float)dkilis) && (reg->currentMA > 0.000001 * (float)dkilis)) 
      || (reg->currentMA <= 0.000001 * (float)dkilis))
      {
        // TODO Fix status frequent change on lower current
        hwStatus = OUT_STATUS_OPEN_LOAD;
      }
      // 12000 = 12V
      else if((reg->voltageMV <= batteryVoltage) && (reg->currentMA > 0.0000143 * (float)dkilis))
      {
        hwStatus = OUT_STATUS_OK;
      }
  }
  /// STATE DETECTION BLOCK END

  vPortEnterCritical();
  // TODO Change this to proper state machine
  // TODO Value such over current or over temperature (even software one) should be hold till restart
  T_OUT_STATUS prevStatus = reg->status;
  if(newStatus == OUT_STATUS_SOFT_OC && hwStatus == OUT_STATUS_HARD_OC_OR_OT)
  {
   reg->status =  OUT_STATUS_HARD_OC_OR_OT;
  }else if(newStatus == OUT_STATUS_SOFT_OC)
  {
    reg->status = newStatus;
  }else
  {
    reg->status = hwStatus;
  }

  if( reg->status != prevStatus )
  {
    if(reg->status == OUT_STATUS_HARD_OC_OR_OT ||
        reg->status == OUT_STATUS_SOFT_OC ||
        reg->status == OUT_STATUS_S_AND_H_OC)
    {
      OUT_DIAG_OnErrorFallback(id);
    }
  }
  vPortExitCritical();
}

// FAST CONTROL LOOP BODY
void OUT_DIAG_AllBts(void)
{
  uint32_t t1 = DEBUG_ARM_GET_TIME;
  for(uint8_t id = 0; id < OUT_ID_BTS_MAX; id++)
  {
    OUT_DIAG_SingleBts(id);
  }
  uint32_t t2 = DEBUG_ARM_GET_TIME;
  uint32_t delta = DEBUG_ARM_CLOCKS_TO_US(t2 - t1);
  LOG_VAR(delta);
}

/// @brief Perform all needed processing for output channel of SPOC2 type
/// @param id Output channel id [9..16] T_OUT_ID
static void OUT_DIAG_SingleSpoc(T_OUT_ID id)
{
  // TODO Requires major rework
  OUT_ASSERT_IN_RANGE(id);
  
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  // SPOC2 ONLY
  if( outsCfg[id].type != OUT_TYPE_SPOC2)
  {
    return;
  }
  
  // TODO VMUX_GetValue in two seperate sections? Here and in bts (does it make sense?)
  reg->voltageMV = VMUX_GetValue(id);
  reg->currentMA = BSP_OUT_CalcCurrent(id);
  //uint32_t batteryVoltage = VMUX_GetBattValue();
  //uint32_t dkilis = BSP_OUT_GetDkilis(id);
  
  if(reg->state == OUT_STATE_ON)
  {
    reg->status = OUT_STATUS_OK;
  }
  else if(reg->state == OUT_STATE_OFF)
  {
    reg->status = OUT_STATUS_OK;
  }
  else if(reg->state == OUT_STATE_ERR_LATCH)
  {
    reg->status = OUT_STATE_ERR_LATCH;
  }

  return;   
}

void OUT_DIAG_AllSpoc(void)
{
  for(uint8_t id = OUT_ID_BTS_MAX; id < OUT_ID_MAX; id++)
  {
    OUT_DIAG_SingleSpoc(id);
  }
}

uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->currentMA;
}

uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->currentMA / 10;
}

uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->voltageMV;
}

T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->status;
}

T_OUT_STATE OUT_DIAG_GetState(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->state;
}

#pragma endregion DIAG_SECTION

T_OUT_TYPE OUT_GetType(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETCFGPTR(id)->type;
}

char* OUT_GetName(T_OUT_ID id)
{
 OUT_ASSERT_IN_RANGE(id);
 return OUT_GETCFGPTR(id)->name;
}

void testTaskEntry(void *argument)
{
  for(;;)
  {
    osDelay(10);
  }
  // /* TODO There is possibility to add UT here for setting output mode and setting output state */
  // T_OUT_CFG* out = OUT_GetCfgPtr( OUT_ID_1 );

  // OUT_ChangeMode( out, OUT_MODE_STD ); 
  // //OUT_SetState( out, OUT_STATE_ON );
  // for (;;)
  // {
  //   OUT_ToggleState( out );
  //   osDelay(3000);
  // }

  // SPOC2_Init();
  // OUT_ChangeMode(OUT_ID_1, OUT_MODE_PWM);
  // for(;;)
  // {
  //   for(uint8_t i = 0; i < 100; i++)
  //   {
  //     BSP_OUT_SetDutyPWM(OUT_ID_1, i);
  //     osDelay(10);
  //   }

  // }

  // ADC_ChannelConfTypeDef sConfig = {0};
  // sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
  // sConfig.Channel = ADC_CHANNEL_6;
  // sConfig.Rank = 2;
  // if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  // {
  //   Error_Handler();
  // }
  // HAL_ADC_Start(&hadc3);
  // HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
  // adc_val = HAL_ADC_GetValue(&hadc3);

  // for( T_OUT_ID i = OUT_ID_1; i < OUT_ID_MAX; i++)
  // {
  //   OUT_ChangeMode( i, OUT_MODE_STD);
  // }

  // for(;;)
  // {
  //   for( T_OUT_ID i = OUT_ID_1; i < OUT_ID_MAX; i++)
  //   {
  //     OUT_ToggleState(i);
  //   }
  //   osDelay(2000);
  //   //SPOC2_SelectSenseMux(SPOC2_ID_1, 0);
  // }
}

///
/// TODO [MAJOR REWORK] Seperate output control code from safety functions, move safety functions into blocks, 
/// safety on seperate core or assure that it is safe to run always (on hardware timer as critical section), 
/// Status and state calculations should be done more properly
/// Add signal filtering on current and voltage signals
/// Handle PWM  signals
/// Add safety detection as status or state
/// Assure safety retry correct working
/// Remove this 4x shit from OC trip
/// Sync current and voltage 
/// Change RTOS tasks names (remove entry nomencalture)
/// Read all and clean up defines