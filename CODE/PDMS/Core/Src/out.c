#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "logger.h"
#include "llgpio.h"
#include "out.h"

T_OUT_CFG outsCfg[] = 
{
    [0] = {
        .id = OUT_ID_1,
        .io = { GPIOC, 9},
        .mode = OUT_MODE_UNUSED,
        .state = OUT_STATE_OFF,
    },
    [1] = {
        .id = OUT_ID_2,
        .io = { GPIOA, 10},
        .mode = OUT_MODE_UNUSED, 
        .state = OUT_STATE_OFF
    }
};


//Remember this function will reinit even if targetMode == currentMode
void OUT_ChangeMode(T_OUT_CFG* cfg, T_OUT_MODE targetMode)
{
    ASSERT(cfg);

    switch (targetMode)
    {
    case OUT_MODE_UNUSED:
    {
        if( cfg->mode == OUT_MODE_PWM )
        {
            LLGPIO_DeInitPWM(cfg->io);
        }
        LLGPIO_SetMode(cfg->io, LL_GPIO_MODE_ANALOG);
        break;
    }
    case OUT_MODE_STD:
    {
        if( cfg->mode == OUT_MODE_PWM )
        {
            LLGPIO_DisablePWM(cfg->io);
        }
        LLGPIO_SetMode(cfg->io, LL_GPIO_MODE_OUTPUT);
        OUT_SetState(cfg, OUT_STATE_OFF);
        break;
    }
    case OUT_MODE_PWM:
    {  
        LLGPIO_InitPWM(cfg->io);
        //Set PWM duty to 0%
        LLGPIO_SetDutyPWM(cfg->io, 0);
        break;
    }
    case OUT_MODE_BATCH:
    {
        LOG_WARN("Batching two inputs!");

        /* Batch shouldn't be out of range */
        ASSERT( cfg->batch < ARRAY_COUNT(outsCfg));

        T_OUT_CFG* batchCfg = &(outsCfg[cfg->batch]);        
        
        OUT_ChangeMode( cfg, OUT_MODE_STD);
        OUT_ChangeMode( batchCfg, OUT_MODE_STD);

        batchCfg->mode = OUT_MODE_BATCH;
        batchCfg->batch = cfg->id;

        OUT_SetState(cfg, OUT_STATE_OFF);

        /* From now on actions on batch outputs are performed simultaniously */
        break;
    }

    default:
        break;
    }

    cfg->mode = targetMode;
    cfg->state = OUT_STATE_OFF;
}

bool OUT_SetState(T_OUT_CFG* cfg, T_OUT_STATE state)
{
    bool res = TRUE;

    ASSERT(cfg);
    
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
        LLGPIO_SetStdState(cfg->io, state);

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
        LLGPIO_SetStdState(cfg->io, TRUE);

        ASSERT( cfg->batch < ARRAY_COUNT(outsCfg));

        T_OUT_CFG* batchCfg = &(outsCfg[cfg->batch]);

        LLGPIO_SetStdState(batchCfg->io, TRUE);
        
        cfg->state = state;
        batchCfg->state = state;
        res = TRUE;
        break;
    }
    default:
        break;
    }
}

bool OUT_AttachAdditional(T_OUT_CFG* cfg, T_OUT_ID batch)
{
    bool res = TRUE;

    ASSERT(cfg);

    if(cfg->mode != OUT_MODE_STD)
    {
        LOG_ERR("Unable to set batch on output not std!");
        return FALSE;
    }

    if(batch >= ARRAY_COUNT(outsCfg))
    {
        LOG_ERR("Setting batch out of range!");
        return FALSE;
    }

    cfg->batch = batch;
}