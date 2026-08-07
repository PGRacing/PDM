#include "main.h"
#include "logger.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"

/// USER DEFINES
#define IN_VALUE_UNUSED 0xFFFF
#define IN_INVALID_ID 0xFFFF

/// FUNCTION PROTOTYPES

/// MACRO FUNCTIONS

// Physical inputs shouldn't be redefined
T_IN_CFG inputsCfg[IN_PHY_MAX + IN_CAN_MAX] =
{
    [0] =
      {
        .location = IN_PHY_LOC_1,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
    },
    [1] =
      {
        .location = IN_PHY_LOC_2,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [2] =
      {
        .location = IN_PHY_LOC_3,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [3] =
      {
        .location = IN_PHY_LOC_4,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_UNUSED,
        },
    [4] =
      {
        .location = IN_PHY_LOC_5,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [5] =
      {
        .location = IN_PHY_LOC_6,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [6] =
      {
        .location = IN_PHY_LOC_7,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
        },
    [7] =
      {
        .location = IN_PHY_LOC_8,
        .type = IN_TYPE_PHY,
        .mode = IN_MODE_SCHMITT,
      },
    [8] = 
      {
        .location = IN_CAN_LOC_1,
        .type = IN_TYPE_CAN,
        .mode = IN_MODE_SCHMITT,
      } 
};

/// @brief Get input configuration pointer by input ID
/// @param id Input ID
/// @return Pointer to input configuration or NULL if invalid ID
T_IN_CFG *IN_GetCfgPtr(T_INPUT_ID id)
{
  ASSERT(id >= 0 && id < ARRAY_COUNT(inputsCfg));
  return &(inputsCfg[id]);
}

/// @brief Get input type by input ID
/// @param id Input ID
/// @return Input type or IN_TYPE_INVALID if invalid ID
T_IN_TYPE IN_GetType(T_INPUT_ID id)
{
  T_IN_CFG* in = IN_GetCfgPtr(id);
  if( in != NULL)
  {
    return in->type; 
  }

  return IN_TYPE_INVALID;
}

/// @brief Check if input is physical by input configuration pointer
/// @param cfg Input configuration pointer
/// @return True if input is physical, False otherwise
bool IN_IsPhysical(T_IN_CFG* cfg)
{
  ASSERT( cfg );

  return (cfg->type == IN_TYPE_PHY);
}

/// @brief Change input mode by input configuration pointer
/// @param cfg Input configuration pointer
/// @param targetMode Target mode to change to
void IN_ChangeMode(T_IN_CFG* cfg, T_IN_MODE targetMode)
{
  ASSERT( cfg );

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

/// @brief Get Schmitt trigger input value by input ID
/// @param id Input ID
/// @return True if input is HIGH, False otherwise
bool IN_GetValueSchmitt(T_INPUT_ID id)
{
  bool ret = FALSE;

  if( IN_GetMode(id) == IN_MODE_SCHMITT )
  {
    if( IN_GetType(id) == IN_TYPE_PHY )
    {
      ret = BSP_PHYIN_GetValueSchmitt(IN_GetCfgPtr(id)->location);
    } 
    else
    {
      ret = BSP_CANIN_GetValueSchmitt(IN_GetCfgPtr(id)->location);
    }
  }
  else
  {
    LOG_WARN( "IN:: trying to get boolean value from non schmitt input");
    ret = FALSE;
  }

  return ret;
}

/// @brief Override CAN input schmitt trigger value by input ID (after usage of previous value)
/// @param id Input ID
/// @param state The new state to set
/// @return TRUE if the override was successful, FALSE otherwise
bool IN_OverrideCANValueSchmitt(T_INPUT_ID id, bool state)
{
    bool ret = FALSE;

  if( IN_GetMode(id) == IN_MODE_SCHMITT && IN_GetType(id) == IN_TYPE_CAN)
  {
    ret = BSP_CANIN_OverrideValueSchmitt(IN_GetCfgPtr(id)->location, state);
  }
  else
  {
    LOG_WARN( "IN:: trying to set boolean value for non CAN schmitt input");
    ret = FALSE;
  }

  return ret;
}

/// @brief Get analog input value by input ID
/// @param id Input ID
/// @return Analog value in mV range [0, 5000]
uint32_t IN_GetValueAnalog(T_INPUT_ID id)
{
  uint32_t ret = 0;

  if( IN_GetMode(id) == IN_MODE_ANALOG )
  {
    if( IN_GetType(id) == IN_TYPE_PHY )
    {
      ret = BSP_PHYIN_GetValueAnalog(IN_GetCfgPtr(id)->location);
    } 
    else
    {
      ret = BSP_CANIN_GetValueAnalog(IN_GetCfgPtr(id)->location);
    }
  }
  else
  {
    LOG_WARN( "IN:: trying to get boolean value from non analog input");
    ret = 0;
  }

  return ret;
}

/// @brief Get input value by input ID
/// @param id Input ID
/// @return Input value based on its mode
uint32_t IN_GetValue(T_INPUT_ID id)
{
  ASSERT(id >= 0 && id < ARRAY_COUNT(inputsCfg));
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

/// @brief Get input mode by input ID
/// @param id Input ID
/// @return Input mode or IN_MODE_UNUSED if invalid ID
T_IN_MODE IN_GetMode(T_INPUT_ID id)
{
  T_IN_CFG* in = IN_GetCfgPtr(id);
  if( in != NULL)
  {
    return in->mode; 
  }

  return IN_MODE_UNUSED;
}