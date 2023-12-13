#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "out.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "logic.h"
// TODO WIP LOGIC

typedef struct
{
  T_LOGIC_VAR_TYPE allowedFirst;
  T_LOGIC_VAR_TYPE allowedSecond;
} T_LOGIC_OPERATOR_RELATION;

const static T_LOGIC_OPERATOR_RELATION operatorsConfig[] =
    {
        [LOGIC_OPERATOR_AND] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_SCHMITT,
                .allowedSecond = LOGIC_VAR_TYPE_SCHMITT},
        [LOGIC_OPERATOR_OR] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_SCHMITT,
                .allowedSecond = LOGIC_VAR_TYPE_SCHMITT},
        [LOGIC_OPERATOR_NE] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANY,
                .allowedFirst = LOGIC_VAR_TYPE_ANY,
            },
        [LOGIC_OPERATOR_E] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANY,
                .allowedFirst = LOGIC_VAR_TYPE_ANY,
            },
        [LOGIC_OPERATOR_G] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
            },
        [LOGIC_OPERATOR_GE] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
            },
        [LOGIC_OPERATOR_L] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
            },
        [LOGIC_OPERATOR_LE] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
                .allowedFirst = LOGIC_VAR_TYPE_ANALOG,
            },
            
        [LOGIC_OPERATOR_IT] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_SCHMITT,
                .allowedFirst = LOGIC_VAR_TYPE_NONE,
            },
            
        [LOGIC_OPERATOR_IF] =
            {
                .allowedFirst = LOGIC_VAR_TYPE_SCHMITT,
                .allowedFirst = LOGIC_VAR_TYPE_NONE,
            },
            
};

static T_LOGIC_VAR_TYPE LOGIC_GetInputModeAsLogicVarType( T_LOGIC_INPUT_TYPE  inType, T_INPUT_ID id)
{
  T_LOGIC_VAR_TYPE result = LOGIC_VAR_TYPE_NONE;

  switch (inType)
  {
  case LOGIC_INPUT_TYPE_SENSOR:
  {
    T_IN_MODE mode = IN_GetMode(id);
    if( mode == IN_MODE_ANALOG)
    {
      result = LOGIC_VAR_TYPE_ANALOG;
    }
    else if(mode == IN_MODE_SCHMITT)
    {
      result = LOGIC_VAR_TYPE_SCHMITT;
    }
    break;
  }
    
  case LOGIC_INPUT_TYPE_CONST_SCHMITT:
  {
    result = LOGIC_VAR_TYPE_SCHMITT;
    break;
  }
  
  case LOGIC_INPUT_TYPE_CONST_ANALOG:
  {
    result = LOGIC_VAR_TYPE_ANALOG;
    break;
  }

  case LOGIC_INPUT_TYPE_UNSET:
  {
    result = LOGIC_VAR_TYPE_NONE;
    break;
  }
   
  default:
    break;
  }

  return result;
}

/* TODO Check if this function works */
static bool LOGIC_IsExpValid( T_LOGIC_EXPRESSION exp )
{
  ASSERT( exp.opr >= 0 );
  ASSERT( exp.opr < LOGIC_OPERATORM_MAX);

  bool res = FALSE;
  T_LOGIC_OPERATOR_RELATION relation = operatorsConfig[exp.opr];

  // Evaluate input types
  if(relation.allowedFirst != LOGIC_VAR_TYPE_ANY 
  && relation.allowedFirst == LOGIC_GetInputModeAsLogicVarType(exp.input1Type, exp.input1ID))
  {
    res = TRUE;
  }
  else if(relation.allowedFirst == LOGIC_VAR_TYPE_ANY)
  {
    res = TRUE;
  }
  else
  {
    res = FALSE;
  }
  
  if(relation.allowedSecond != LOGIC_VAR_TYPE_ANY 
  && relation.allowedSecond == LOGIC_GetInputModeAsLogicVarType(exp.input1Type, exp.input1ID))
  {
    res &= TRUE;
  }
  else if(relation.allowedSecond == LOGIC_VAR_TYPE_ANY)
  {
    res = TRUE;
  }
  else
  {
    res = FALSE;
  }

  
  return res;
}

bool LOGIC_EvaluateExpression (T_LOGIC_EXPRESSION exp)
{
  
  uint32_t r_data1 = 0;
  uint32_t r_data2 = 0;
  bool result = FALSE;

  // Validate expression 
  if( LOGIC_IsExpValid( exp ) == FALSE )
  {
    LOG_WARN("LOGIC: Logic expression invalid!");
    ASSERT( FALSE );
  }

  // Acquire data for first part of expression
  if(exp.input1Type == LOGIC_INPUT_TYPE_SENSOR)
  {
    r_data1 = IN_GetValue(exp.input1ID);
  }
  else if(exp.input1Type == LOGIC_INPUT_TYPE_CONST_SCHMITT 
  || exp.input1Type == LOGIC_INPUT_TYPE_CONST_ANALOG)
  {
    r_data1 = exp.input1Const;
  }

  // Acquire data for second part of expression
  if(exp.input2Type == LOGIC_INPUT_TYPE_SENSOR)
  {
    r_data2 = IN_GetValue(exp.input2ID);
  }
  else if(exp.input2Type == LOGIC_INPUT_TYPE_CONST_SCHMITT 
  || exp.input2Type == LOGIC_INPUT_TYPE_CONST_ANALOG)
  {
    r_data2 = exp.input2Const;
  }
  
  // Evaluate expression
  switch (exp.opr)
  {
  case LOGIC_OPERATOR_AND:
  {
    // A && B
    result = ((bool)r_data1 && (bool)r_data2);
    break;
  }
    
  case LOGIC_OPERATOR_OR:
  {
    // A || B
    result = ((bool)r_data1 || (bool)r_data2);
    break;
  }
    
  case LOGIC_OPERATOR_NE:
  {
    // A != B
    result = ((uint32_t)r_data1 != (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_E:
  {
    // A == B
    result = ((uint32_t)r_data1 == (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_G:
  {
    // A > B
    result = ((uint32_t)r_data1 > (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_GE:
  {
    // A >= B
    result = ((uint32_t)r_data1 >= (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_L:
  {
    // A >= B
    result = ((uint32_t)r_data1 < (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_LE:
  {
    // A >= B
    result = ((uint32_t)r_data1 <= (uint32_t)r_data2);
    break;
  }

  case LOGIC_OPERATOR_IT:
  {
    // A == TRUE
    result = ((bool)r_data1 == TRUE);
    break;
  }

  case LOGIC_OPERATOR_IF:
  {
    // A == FALSE
    result = ((bool)r_data1 == FALSE);
    break;
  }

  default:
  {
    result = FALSE;
    break;
  }
    
  }
  return result;
}

// LOGIC Array struct
T_LOGIC logics[POWER_OUT_COUNT] = 
{
  {
    .isUsed = FALSE
  }
};


volatile bool expResult;

void testTaskEntry(void *argument)
{
  /* TODO There is possibility to add UT here for setting output mode and setting output state */

  T_LOGIC_EXPRESSION exp1 = 
  {
    .opr = LOGIC_OPERATOR_GE,
    .input1Type = LOGIC_INPUT_TYPE_SENSOR,
    .input1ID = IN_PHY_ID_1,
    .input2Type = LOGIC_INPUT_TYPE_CONST_ANALOG,
    .input2Const = 400
  };

  T_LOGIC_EXPRESSION exp2 = 
  {
    .opr = LOGIC_OPERATOR_IT,
    .input1Type = LOGIC_INPUT_TYPE_SENSOR,
    .input1ID = IN_PHY_ID_1,
    .input2Type =  LOGIC_INPUT_TYPE_UNSET,
  };

  OUT_Batch( OUT_ID_1, OUT_ID_2);
  OUT_ChangeMode( OUT_ID_3, OUT_MODE_UNUSED);
  OUT_ChangeMode( OUT_ID_4, OUT_MODE_STD);
  
  OUT_Batch(OUT_ID_5,OUT_ID_8);
  OUT_ChangeMode( OUT_ID_6, OUT_MODE_STD);
  OUT_ChangeMode( OUT_ID_7, OUT_MODE_STD);

  for (;;)
  {
    //expResult = LOGIC_EvaluateExpression( exp2 );
    for( uint8_t i = 0; i < 8; i++)
    {
      OUT_ToggleState(i);
      osDelay(2000);
    }
  }
}