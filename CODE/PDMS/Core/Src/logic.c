#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "llgpio.h"
#include "out.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
// TODO WIP LOGIC

typedef enum
{
  LOGIC_OPERATOR_AND = 0x00,
  LOGIC_OPERATOR_OR = 0x01,
  LOGIC_OPERATOR_NE = 0x02,
  LOGIC_OPERATOR_E = 0x03,
  LOGIC_OPERATOR_G = 0x04,
  LOGIC_OPERATOR_GE = 0x05,
  LOGIC_OPERATOR_L = 0x06,
  LOGIC_OPERATOR_LE = 0x07,
  LOGIC_OPERATOR_IT = 0x08,
  LOGIC_OPERATOR_IF = 0x09,
  LOGIC_OPERATORM_MAX
} T_LOGIC_OPERATOR_ENUM;

typedef enum
{
  // REMEMBER SCHMITT ALSO ALLOWS CONST (TRUE / FALSE )
  LOGIC_EXPRESSION_VAR_SCHMITT = 0x00,
  LOGIC_EXPRESSION_VAR_ANALOG = 0x01,
  LOGIC_EXPRESSION_VAR_ANY = 0x02,
  LOGIC_EXPRESSION_VAR_NONE = 0x03
} T_LOGIC_EXP_VAR;

typedef struct
{
  T_LOGIC_EXP_VAR allowedFirst;
  T_LOGIC_EXP_VAR allowedSecond;
} T_LOGIC_OPERATOR_RELATION;

// TODO add all operators
const T_LOGIC_OPERATOR_RELATION operatorsConfig[] =
    {
        [LOGIC_OPERATOR_AND] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_SCHMITT,
                .allowedSecond = LOGIC_EXPRESSION_VAR_SCHMITT},
        [LOGIC_OPERATOR_OR] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_SCHMITT,
                .allowedSecond = LOGIC_EXPRESSION_VAR_SCHMITT},
        [LOGIC_OPERATOR_NE] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_E] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_G] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_GE] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_L] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_LE] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
                .allowedFirst = LOGIC_EXPRESSION_VAR_ANALOG,
            },
            
        [LOGIC_OPERATOR_IT] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_SCHMITT,
                .allowedFirst = LOGIC_EXPRESSION_VAR_NONE,
            },
            
        [LOGIC_OPERATOR_IF] =
            {
                .allowedFirst = LOGIC_EXPRESSION_VAR_SCHMITT,
                .allowedFirst = LOGIC_EXPRESSION_VAR_NONE,
            },
            
};

typedef struct
{
  T_LOGIC_EXP_VAR var1type;
  void* var1data;
  T_LOGIC_EXP_VAR var2type;
  void* var2data;
  T_LOGIC_OPERATOR_ENUM operator;
} T_LOGIC_EXPRESSION;

/* TODO Check if this function works */
static bool LOGIC_IsExpValid( T_LOGIC_EXPRESSION exp )
{
  ASSERT( exp.operator < LOGIC_OPERATORM_MAX);

  bool res = FALSE;

  T_LOGIC_OPERATOR_RELATION relation = operatorsConfig[exp.operator];

  if( relation.allowedFirst == LOGIC_EXPRESSION_VAR_ANY 
  || exp.var1type == relation.allowedFirst)
  {
    res = TRUE;
  }else
  {
    res = FALSE;
  }

  if( relation.allowedSecond == LOGIC_EXPRESSION_VAR_ANY 
  || exp.var2type == relation.allowedSecond)
  {
    res &= TRUE;
  }else
  {
    res = FALSE;
  }

  // If second != none both types should be same
  if( relation.allowedSecond != LOGIC_EXPRESSION_VAR_NONE
  && relation.allowedFirst != relation.allowedSecond)
  {
    res = FALSE;
  }
  
  return res;
}

bool LOGIC_EvaluateExpression (T_LOGIC_EXPRESSION exp)
{
  if( LOGIC_IsExpValid( exp ) == FALSE )
  {
    LOG_WARN("LOGIC: Logic expression invalid!");
    ASSERT( FALSE );
  }
  
  
  bool result = FALSE;

  switch (exp.operator)
  {
  case LOGIC_OPERATOR_AND:
  {
    // A && B
    result = (*((bool*)exp.var1data) && *((bool*)exp.var2data));
    break;
  }
    
  case LOGIC_OPERATOR_OR:
  {
    // A || B
    result = (*((bool*)exp.var1data) || *((bool*)exp.var2data));
    break;
  }
    
  case LOGIC_OPERATOR_NE:
  {
    // A != B
    result = (*((uint32_t*)exp.var1data) != *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_E:
  {
    // A == B
    result = (*((uint32_t*)exp.var1data) == *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_G:
  {
    // A > B
    result = (*((uint32_t*)exp.var1data) > *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_GE:
  {
    // A >= B
    result = (*((uint32_t*)exp.var1data) >= *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_L:
  {
    // A >= B
    result = (*((uint32_t*)exp.var1data) < *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_LE:
  {
    // A >= B
    result = (*((uint32_t*)exp.var1data) <= *((uint32_t*)exp.var2data));
    break;
  }

  case LOGIC_OPERATOR_IT:
  {
    // A == TRUE
    result = (*((bool*)exp.var1data) == TRUE);
    break;
  }

  case LOGIC_OPERATOR_IF:
  {
    // A == FALSE
    result = (*((bool*)exp.var1data) == FALSE);
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

volatile bool expResult;

void testTaskEntry(void *argument)
{
  /* TODO There is possibility to add UT here for setting output mode and setting output state */
  uint32_t a = 2000;

  T_LOGIC_EXPRESSION exp1 = 
  {
    .operator = LOGIC_OPERATOR_GE,
    .var1type = LOGIC_EXPRESSION_VAR_ANALOG,
    .var1data = inputsCfg[0].rawData,
    .var2type = LOGIC_EXPRESSION_VAR_ANALOG,
    .var2data = &a
  };

  OUT_ChangeMode( OUT_GetPtr(OUT_ID_1), OUT_MODE_STD);

  for (;;)
  {
    expResult = LOGIC_EvaluateExpression( exp1 );
    OUT_SetState( OUT_GetPtr(OUT_ID_1), expResult);
    osDelay(10);
  }
}