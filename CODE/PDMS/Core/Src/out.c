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

#define OUT_MAX 8

T_OUT_CFG outsCfg[OUT_MAX] =
    {
        [OUT_ID_1] = {
            .id = OUT_ID_1,
            .io = {PWM_SIG1_GPIO_Port, PWM_SIG1_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_2] = {
            .id = OUT_ID_2,
            .io = {PWM_SIG2_GPIO_Port, PWM_SIG2_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_3] = {
            .id = OUT_ID_3,
            .io = {PWM_SIG3_GPIO_Port, PWM_SIG3_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_4] = {
            .id = OUT_ID_4,
            .io = {PWM_SIG4_GPIO_Port, PWM_SIG4_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_5] = {
            .id = OUT_ID_5,
            .io = {PWM_SIG5_GPIO_Port, PWM_SIG5_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_6] = {
            .id = OUT_ID_6,
            .io = {PWM_SIG6_GPIO_Port, PWM_SIG6_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_7] = {
            .id = OUT_ID_7,
            .io = {PWM_SIG7_GPIO_Port, PWM_SIG7_Pin},
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF,
        },
        [OUT_ID_8] = {
            .id = OUT_ID_8,
            .io = {PWM_SIG8_GPIO_Port, PWM_SIG8_Pin},
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
      BSP_OUT_DeInitPWM(cfg->io);
    }
    BSP_OUT_SetMode(cfg->io, targetMode);
    break;
  }
  case OUT_MODE_STD:
  {
    if (cfg->mode == OUT_MODE_PWM)
    {
      BSP_OUT_DeInitPWM(cfg->io);
    }
    BSP_OUT_SetMode(cfg->io, targetMode);
    OUT_SetState(cfg->id, OUT_STATE_OFF);
    break;
  }
  case OUT_MODE_PWM:
  {
    BSP_OUT_InitPWM(cfg->io);
    // Set PWM duty to 0%
    BSP_OUT_SetDutyPWM(cfg->io, 0);
    break;
  }
  case OUT_MODE_BATCH:
  {
    LOG_WARN("Batching two inputs!");

    /* Batch shouldn't be out of range */
    ASSERT(cfg->batchId < ARRAY_COUNT(outsCfg));
    T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

    // LL MODE STD
    BSP_OUT_SetMode(cfg->io, OUT_MODE_STD);
    BSP_OUT_SetMode(batchCfg->io, OUT_MODE_STD);
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
    BSP_OUT_SetStdState(cfg->io, state);

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

    BSP_OUT_SetBatchState(cfg->io, batchCfg->io, state);

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

  if( cfg->io.port != batchCfg->io.port)
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

// void testTaskEntry(void *argument)
// {
//   /* TODO There is possibility to add UT here for setting output mode and setting output state */
//   T_OUT_CFG* out = OUT_GetPtr( OUT_ID_1 );

//   OUT_ChangeMode( out, OUT_MODE_STD ); 
//   //OUT_SetState( out, OUT_STATE_ON );
//   for (;;)
//   {
//     OUT_ToggleState( out );
//     osDelay(3000);
//   }
// }

// TODO Add OUT_SetPWMDuty(T_OUT_CFG *cfg, T_OUT_STATE state)