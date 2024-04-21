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

// Safety related defines
#define OUT_DIAG_READ_FREQ 200 // 200 Hz frequency of adc read
#define OUT_DIAG_READ_PERIOD 1000 / OUT_DIAG_READ_FREQ
#define OUT_SAFETY_OC_OFF 0xFFFF
#define OUT_DIAG_MS_TO_OC_TRIP(X) X / OUT_DIAG_READ_PERIOD
//#define OUT_DIAG_BTS500_ADC_VOLTAGE_TO_MA

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
            .safety = 
            {
              .useOc = true,
              .ocThreshold = 2000, // 1000mA Threshold
              .ocTripCounter = 0,
              .ocTripThreshold = 0
            }
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
        [OUT_ID_9] = {
            .id = OUT_ID_9,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },
        [OUT_ID_10] = {
            .id = OUT_ID_10,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },
        [OUT_ID_11] = {
            .id = OUT_ID_11,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },
        [OUT_ID_12] = {
            .id = OUT_ID_12,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },
        [OUT_ID_13] = {
            .id = OUT_ID_13,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },        
        [OUT_ID_14] = {
            .id = OUT_ID_14,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },        
        [OUT_ID_15] = {
            .id = OUT_ID_15,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },        
        [OUT_ID_16] = {
            .id = OUT_ID_16,
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
            .state = OUT_STATE_OFF
        },        

};

// Faster macro way (w/unsafe)
#define OUT_GETPTR(id) &(outsCfg[id])

T_OUT_CFG* OUT_GetPtr( T_OUT_ID id )
{
  ASSERT( id >= 0 && id < ARRAY_COUNT(outsCfg) );
  return &(outsCfg[id]);
}

// Remember this function will reinit even if targetMode == currentMode
void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode)
{
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  ASSERT( cfg );
  ASSERT( cfg->id >= 0 && cfg->id < ARRAY_COUNT(outsCfg) );

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

  cfg->state = OUT_STATE_OFF;
}

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE state)
{
  bool res = TRUE;
  T_OUT_CFG* cfg = OUT_GetPtr(id);

  ASSERT(cfg);
  ASSERT( cfg->id >= 0 && cfg->id < ARRAY_COUNT(outsCfg) );

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
      BSP_OUT_SetStdState(id, state);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      SPOC2_SetStdState(outsCfg[id].spocId, outsCfg[id].spocChId, state);
    }
    
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
    if(cfg->type == OUT_TYPE_BTS500)
    {
      ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));

      T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

      BSP_OUT_SetBatchState(id, cfg->batch, state);

      cfg->state = state;
      batchCfg->state = state;
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

/* Note: For BTS500 you can batch between 1-4 and 5-8*/
bool OUT_Batch(T_OUT_ID id, T_OUT_ID batchId)
{
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  T_OUT_CFG* batchCfg = OUT_GetPtr(batchId);
  ASSERT(cfg);
  ASSERT( cfg->id >= 0 && cfg->id < ARRAY_COUNT(outsCfg) );

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
  T_OUT_CFG* cfg = OUT_GetPtr(id);
  return OUT_SetState( id, !cfg->state);
}

static void OUT_DIAG_OnSoftOC(T_OUT_ID id)
{
  OUT_SetState(id, OUT_STATE_OFF);
}


static void OUT_DIAG_Single(T_OUT_ID id)
{
  ASSERT( id < ARRAY_COUNT(outsCfg) );
  
  T_OUT_CFG* cfg = OUT_GETPTR(id);
  T_OUT_STATUS newStatus = OUT_STATUS_NORMAL_OFF;

  // Currently for BTS500 only
  if( outsCfg[id].type != OUT_TYPE_BTS500)
  {
    return;
  }

  cfg->currentMA = BSP_OUT_CalcCurrent(id);
  
  // For now this is done first
  if(true == cfg->safety.useOc)
  {
    if(cfg->currentMA > cfg->safety.ocThreshold)
    {
      if(cfg->safety.ocTripCounter >= cfg->safety.ocTripThreshold)
      {
        OUT_DIAG_OnSoftOC(id);
        newStatus = OUT_STATUS_SOFT_OC;
        cfg->safety.ocTripCounter = 0;
      }else
      {
        if(cfg->state == OUT_STATE_ON)
        {
          cfg->safety.ocTripCounter++;
        }
      }
    }
  }
  
  cfg->voltageMV = VMUX_GetValue(id);
  //uint32_t fault_level = BSP_OUT_GetDkilis(id) * 4;
  uint32_t batteryVoltage = VMUX_GetBattValue();
  uint32_t faultLevel = 12000;
  uint32_t dkilis = BSP_OUT_GetDkilis(id);
  uint32_t voltageHis = 500; // 500 mV for now

  T_OUT_STATUS hwStatus = OUT_STATUS_NORMAL_OFF;
  if(OUT_STATE_OFF == cfg->state)
  {
      if(cfg->currentMA >= faultLevel)
      { 
        if(abs((int32_t)(batteryVoltage - cfg->voltageMV) < voltageHis))
        {
          hwStatus = OUT_STATUS_SHORT_TO_VSS;
        }
        else
        {
          hwStatus = OUT_STATUS_NORMAL_OFF;
        }
      }
  }
  else
  {
    // TODO Fix magic values
      if(cfg->currentMA >= faultLevel)
      {
        hwStatus = OUT_STATUS_HARD_OC_OR_OT;
      }
      else if(((cfg->currentMA <= 0.0000143 * (float)dkilis) && (cfg->currentMA > 0.000001 * (float)dkilis)) 
      || (cfg->currentMA <= 0.000001 * (float)dkilis))
      {
        hwStatus = OUT_STATUS_OPEN_LOAD;
      }
      // 12000 = 12V
      else if((cfg->voltageMV <= batteryVoltage) && (cfg->currentMA > 0.0000143 * (float)dkilis))
      {
        hwStatus = OUT_STATUS_NORMAL_ON;
      }
  }

  if(newStatus == OUT_STATUS_SOFT_OC && hwStatus == OUT_STATUS_HARD_OC_OR_OT)
  {
   cfg->status =  OUT_STATUS_HARD_OC_OR_OT;
  }else if(newStatus == OUT_STATUS_SOFT_OC)
  {
    cfg->status = newStatus;
  }else
  {
    cfg->status = hwStatus;
  }
}

void OUT_DIAG_All()
{
  for(uint8_t id = 0; id < OUT_ID_MAX; id++)
  {
    OUT_DIAG_Single(id);
  }
}

uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id)
{
  ASSERT(id < OUT_ID_MAX);
  return outsCfg[id].currentMA;
}

uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id)
{
  ASSERT(id < OUT_ID_MAX);
  return outsCfg[id].currentMA / 10;
}

uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id)
{
  ASSERT(id < OUT_ID_MAX);
  return outsCfg[id].voltageMV;
}

T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id)
{
  ASSERT(id < OUT_ID_MAX);
  return outsCfg[id].status;
}

void testTaskEntry(void *argument)
{
  for(;;)
  {

  }
  // /* TODO There is possibility to add UT here for setting output mode and setting output state */
  // T_OUT_CFG* out = OUT_GetPtr( OUT_ID_1 );

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

// TODO Add OUT_SetPWMDuty(T_OUT_CFG *cfg, T_OUT_STATE state)