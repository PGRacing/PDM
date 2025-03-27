#include "main.h"
#include "logger.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"

/// USER DEFINES
#define IN_VALUE_UNUSED 0xFFFF

/// FUNCTION PROTOTYPES

/// MACRO FUNCTIONS

// Physical inputs shouldn't be redefined
T_IN_CFG inputsCfg[] =
{
    [0] =
      {
        .id = IN_PHY_ID_1,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
    },
    [1] =
      {
        .id = IN_PHY_ID_2,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [2] =
      {
        .id = IN_PHY_ID_3,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [3] =
      {
        .id = IN_PHY_ID_4,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [4] =
      {
        .id = IN_PHY_ID_5,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [5] =
      {
        .id = IN_PHY_ID_6,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [6] =
      {
        .id = IN_PHY_ID_7,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [7] =
      {
        .id = IN_PHY_ID_8,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
      },
};

T_IN_CFG *IN_GetCfgPtr(T_INPUT_ID id)
{
  // TODO Extend for can inputs
  ASSERT(id >= 0 && id < ARRAY_COUNT(inputsCfg));
  return &(inputsCfg[id]);
}

T_IN_TYPE IN_GetType(T_INPUT_ID id)
{
  T_IN_CFG* in = IN_GetCfgPtr(id);
  if( in != NULL)
  {
    return in->type; 
  }

  return IN_TYPE_INVALID;
}

bool IN_IsPhysical(T_IN_CFG* cfg)
{
  ASSERT( cfg );
  ASSERT( cfg->id >= 0 && cfg->id < ARRAY_COUNT(inputsCfg) );

  if( cfg->id < IN_PHY_MAX )
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

void IN_ChangeMode(T_IN_CFG* cfg, T_IN_MODE targetMode)
{
  ASSERT( cfg );
  ASSERT( cfg->id >= 0 && cfg->id < ARRAY_COUNT(inputsCfg));

  switch (targetMode)
  {
    case IN_MODE_UNUSED:
    case IN_MODE_ANALOG:
    case IN_MODE_SCHMITT:
    {
      // Do nothing (for now no phy changes)
      break;
    }

    case IN_MODE_OUTPUT:
    {
      ASSERT( FALSE );
      /* Not supported */
      return; 
    }
  }

  cfg->mode = targetMode;

}

bool IN_GetValueSchmitt(T_INPUT_ID id)
{
  bool ret = FALSE;

  if( IN_GetMode(id) == IN_MODE_SCHMITT )
  {
    if( IN_GetType(id) == IN_TYPE_PHY )
    {
      ret =  BSP_IN_GetValueSchmitt(id);
    } 
    else
    {
      LOG_WARN( "IN:: NOT IMPLEMENTED trying to get value from non physical input");
      ret = FALSE;
    }
  }
  else
  {
    LOG_WARN( "IN:: trying to get boolean value from non schmitt input");
    ret = FALSE;
  }

  return ret;
}

uint32_t IN_GetValueAnalog(T_INPUT_ID id)
{
  bool ret = 0;

  if( IN_GetMode(id) == IN_MODE_ANALOG )
  {
    if( IN_GetType(id) == IN_TYPE_PHY )
    {
      ret =  BSP_IN_GetValueAnalog(id);
    } 
    else
    {
      LOG_WARN( "IN:: NOT IMPLEMENTED trying to get value from non physical input");
      ret = 0;
    }
  }
  else
  {
    LOG_WARN( "IN:: trying to get boolean value from non analog input");
    ret = 0;
  }

  return ret;
}

uint32_t IN_GetValue(T_INPUT_ID id)
{
  T_IN_CFG* in = IN_GetCfgPtr(id);
  uint16_t ret = 0x0;

  switch (in->mode)
  {
  case IN_MODE_ANALOG:
    ret = IN_GetValueAnalog(id);
    break;
  
  case IN_MODE_SCHMITT:
    ret = IN_GetValueSchmitt(id);
    break;

  case IN_MODE_UNUSED:
    ret = IN_VALUE_UNUSED;
    break; 

  default:
    break;
  }
  
  return ret;
}

T_IN_MODE IN_GetMode(T_INPUT_ID id)
{
  T_IN_CFG* in = IN_GetCfgPtr(id);
  if( in != NULL)
  {
    return in->mode; 
  }

  return IN_MODE_UNUSED;
}


///
/// TODO [MAJOR REWORK] Add handling of can inputs
///