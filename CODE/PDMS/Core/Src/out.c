#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "out.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "spoc2.h"

T_OUT_CFG outsCfg[OUT_ID_MAX] =
    {
        [OUT_ID_1] = {
            .id = OUT_ID_1,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_2] = {
            .id = OUT_ID_2,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_3] = {
            .id = OUT_ID_3,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_4] = {
            .id = OUT_ID_4,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_5] = {
            .id = OUT_ID_5,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_6] = {
            .id = OUT_ID_6,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_7] = {
            .id = OUT_ID_7,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_8] = {
            .id = OUT_ID_8,
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
};

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id )
{
  ASSERT( id > 0 && id < ARRAY_COUNT(outsCfg) );
  return &(outsCfg[id]);
}

// Remember this function will reinit even if targetMode == currentMode
void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode)
{
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  ASSERT( cfg );
  ASSERT( cfg->id > 0 && cfg->id < ARRAY_COUNT(outsCfg) );

  switch (targetMode)
  {
  case OUT_MODE_UNUSED:
  {
    if (cfg->mode == OUT_MODE_PWM)
    {
      BSP_OUT_DeInitPWM(id);
    }
    BSP_OUT_SetMode(id, targetMode);
    break;
  }
  case OUT_MODE_STD:
  {
    if (cfg->mode == OUT_MODE_PWM)
    {
      BSP_OUT_DeInitPWM(id);
    }
    BSP_OUT_SetMode(id, targetMode);
    OUT_SetState(cfg->id, OUT_STATE_OFF);
    break;
  }
  case OUT_MODE_PWM:
  {
    BSP_OUT_SetMode(id, targetMode);
    // Set PWM duty to 0%
    BSP_OUT_SetDutyPWM(id, 0);
    break;
  }
  case OUT_MODE_BATCH:
  {
    LOG_WARN("Batching two inputs!");

    /* Batch shouldn't be out of range */
    ASSERT(cfg->batchId < ARRAY_COUNT(outsCfg));
    T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

    // LL MODE STD
    BSP_OUT_SetMode(id, OUT_MODE_STD);
    BSP_OUT_SetMode(cfg->batch, OUT_MODE_STD);
    OUT_SetState(cfg->id, OUT_STATE_OFF);
    OUT_SetState(batchCfg->id, OUT_STATE_OFF);
    batchCfg->mode = OUT_MODE_BATCH;
    /* From now on actions on batch outputs are performed simultaniously */
    break;
  }

  default:
    break;
  }

  cfg->mode = targetMode;
  cfg->state = OUT_STATE_OFF;
}

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE state)
{
  bool res = TRUE;
  T_OUT_CFG* cfg = OUT_GetPtr(id);

  ASSERT(cfg);
  ASSERT( cfg->id > 0 && cfg->id < ARRAY_COUNT(outsCfg) );

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
    BSP_OUT_SetStdState(id, state);

    cfg->state = state;
    res = TRUE;
    break;
  }
  case OUT_MODE_PWM:
  {
    LOG_WARN("Trying to set steady state on PWM output");
    res = FALSE;
    break;
  }
  case OUT_MODE_BATCH:
  {
    ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));

    T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

    BSP_OUT_SetBatchState(id, cfg->batch, state);

    cfg->state = state;
    batchCfg->state = state;
    res = TRUE;
    break;
  }
  default:
    break;
  }

  return res;
}

/* Note: For BTS500 you can batch between 1-4 and 5-8*/
bool OUT_Batch(T_OUT_ID id, T_OUT_ID batchId)
{
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  T_OUT_CFG* batchCfg = OUT_GetPtr(batchId);
  ASSERT(cfg);
  ASSERT( cfg->id > 0 && cfg->id < ARRAY_COUNT(outsCfg) );

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
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  return OUT_SetState( id, !cfg->state);
}

SPOC2_chain_t chain = {
    .devices = {
        {
            .deviceIdentifier = BTS72220_4ESA,
            .adcChannel = 0,
        },
        {
            .deviceIdentifier = BTS72220_4ESA,
            .adcChannel = 1,
        },
    },
    .numDevices = 2,
    .spiBusId = 0,
    .spiChipSelect = 0,
};

void testTaskEntry(void *argument)
{
  // /* TODO There is possibility to add UT here for setting output mode and setting output state */
  // T_OUT_CFG* out = OUT_GetPtr( OUT_ID_1 );

  // OUT_ChangeMode( out, OUT_MODE_STD ); 
  // //OUT_SetState( out, OUT_STATE_ON );
  // for (;;)
  // {
  //   OUT_ToggleState( out );
  //   osDelay(3000);
  // }

  SPOC2_Chain_init(&chain);
  SPOC2_Config_disableSleepMode(&chain.devices[0]);
  SPOC2_Config_disableSleepMode(&chain.devices[1]);
  SPOC2_Config_enableOutputChannel(&chain.devices[0], 0);
  SPOC2_Config_enableOutputChannel(&chain.devices[0], 1);
  SPOC2_Config_enableOutputChannel(&chain.devices[1], 2);
  SPOC2_Chain_applyDeviceConfigs(&chain);
}

// TODO Add OUT_SetPWMDuty(T_OUT_CFG *cfg, T_OUT_STATE state)