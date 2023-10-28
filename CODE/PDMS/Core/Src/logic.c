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
#include "logic.h"
// TODO WIP LOGIC

typedef struct
{
  T_LOGIC_VAR allowedFirst;
  T_LOGIC_VAR allowedSecond;
} T_LOGIC_OPERATOR_RELATION;

const T_LOGIC_OPERATOR_RELATION operatorsConfig[] =
    {
        [LOGIC_OPERATOR_AND] =
            {
                .allowedFirst = LOGIC_VAR_SCHMITT,
                .allowedSecond = LOGIC_VAR_SCHMITT},
        [LOGIC_OPERATOR_OR] =
            {
                .allowedFirst = LOGIC_VAR_SCHMITT,
                .allowedSecond = LOGIC_VAR_SCHMITT},
        [LOGIC_OPERATOR_NE] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_E] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_G] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_GE] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_L] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
        [LOGIC_OPERATOR_LE] =
            {
                .allowedFirst = LOGIC_VAR_ANALOG,
                .allowedFirst = LOGIC_VAR_ANALOG,
            },
            
        [LOGIC_OPERATOR_IT] =
            {
                .allowedFirst = LOGIC_VAR_SCHMITT,
                .allowedFirst = LOGIC_VAR_NONE,
            },
            
        [LOGIC_OPERATOR_IF] =
            {
                .allowedFirst = LOGIC_VAR_SCHMITT,
                .allowedFirst = LOGIC_VAR_NONE,
            },
            
};

// /* TODO Check if this function works */
// static bool LOGIC_IsExpValid( T_LOGIC_EXPRESSION exp )
// {
//   ASSERT( exp.opr < LOGIC_OPERATORM_MAX);

//   bool res = FALSE;

//   T_LOGIC_OPERATOR_RELATION relation = operatorsConfig[exp.opr];

//   if( relation.allowedFirst == LOGIC_VAR_ANY 
//   || exp.var1type == relation.allowedFirst)
//   {
//     res = TRUE;
//   }else
//   {
//     res = FALSE;
//   }

//   if( relation.allowedSecond == LOGIC_VAR_ANY 
//   || exp.var2type == relation.allowedSecond)
//   {
//     res &= TRUE;
//   }else
//   {
//     res = FALSE;
//   }

//   // If second != none both types should be same
//   if( relation.allowedSecond != LOGIC_VAR_NONE
//   && relation.allowedFirst != relation.allowedSecond)
//   {
//     res = FALSE;
//   }
  
//   return res;
// }

// bool LOGIC_EvaluateExpression (T_LOGIC_EXPRESSION exp)
// {
//   /* TODO Here we should use INPUT_ID or constant instead of pointers to variable */
//   if( LOGIC_IsExpValid( exp ) == FALSE )
//   {
//     LOG_WARN("LOGIC: Logic expression invalid!");
//     ASSERT( FALSE );
//   }
  
//   bool result = FALSE;

//   switch (exp.opr)
//   {
//   case LOGIC_OPERATOR_AND:
//   {
//     // A && B
//     result = (*((bool*)exp.var1data) && *((bool*)exp.var2data));
//     break;
//   }
    
//   case LOGIC_OPERATOR_OR:
//   {
//     // A || B
//     result = (*((bool*)exp.var1data) || *((bool*)exp.var2data));
//     break;
//   }
    
//   case LOGIC_OPERATOR_NE:
//   {
//     // A != B
//     if ( exp.var1type == LOGIC_VAR_ANALOG &&
//           exp.var2type == LOGIC_VAR_ANALOG)
//           {
//             result = (*((uint32_t*)exp.var1data) != *((uint32_t*)exp.var2data));
//           }
//           else if( exp.var1type == LOGIC_VAR_SCHMITT &&
//           exp.var2type == LOGIC_VAR_SCHMITT )
//           {
//             result = (*((bool*)exp.var1data) != *((bool*)exp.var2data));
//           }
    
//     break;
//   }

//   case LOGIC_OPERATOR_E:
//   {
//     // A == B
//     if ( exp.var1type == LOGIC_VAR_ANALOG &&
//       exp.var2type == LOGIC_VAR_ANALOG)
//       {
//         result = (*((uint32_t*)exp.var1data) == *((uint32_t*)exp.var2data));
//       }
//       else if( exp.var1type == LOGIC_VAR_SCHMITT &&
//       exp.var2type == LOGIC_VAR_SCHMITT )
//       {
//         result = (*((bool*)exp.var1data) == *((bool*)exp.var2data));
//       }
//     break;
//   }

//   case LOGIC_OPERATOR_G:
//   {
//     // A > B
//     result = (*((uint32_t*)exp.var1data) > *((uint32_t*)exp.var2data));
//     break;
//   }

//   case LOGIC_OPERATOR_GE:
//   {
//     // A >= B
//     result = (*((uint32_t*)exp.var1data) >= *((uint32_t*)exp.var2data));
//     break;
//   }

//   case LOGIC_OPERATOR_L:
//   {
//     // A >= B
//     result = (*((uint32_t*)exp.var1data) < *((uint32_t*)exp.var2data));
//     break;
//   }

//   case LOGIC_OPERATOR_LE:
//   {
//     // A >= B
//     result = (*((uint32_t*)exp.var1data) <= *((uint32_t*)exp.var2data));
//     break;
//   }

//   case LOGIC_OPERATOR_IT:
//   {
//     // A == TRUE
//     result = (*((bool*)exp.var1data) == TRUE);
//     break;
//   }

//   case LOGIC_OPERATOR_IF:
//   {
//     // A == FALSE
//     result = (*((bool*)exp.var1data) == FALSE);
//     break;
//   }

//   default:
//   {
//     result = FALSE;
//     break;
//   }
    
//   }
//   return result;
// }

// // LOGIC Array struct
// T_LOGIC logics[POWER_OUT_COUNT] = 
// {
//   {
//     .isUsed = FALSE
//   }
// };


volatile bool expResult;

// void testTaskEntry(void *argument)
// {
//   /* TODO There is possibility to add UT here for setting output mode and setting output state */
//   uint32_t a = 2000;

//   T_LOGIC_EXPRESSION exp1 = 
//   {
//     .opr = LOGIC_OPERATOR_GE,
//     .var1type = LOGIC_VAR_ANALOG,
//     .var1data = inputsCfg[0].rawData,
//     .var2type = LOGIC_VAR_ANALOG,
//     .var2data = (&a)
//   };

//   OUT_ChangeMode( OUT_GetPtr(OUT_ID_1), OUT_MODE_STD);

//   for (;;)
//   {
//     //expResult = LOGIC_EvaluateExpression( exp1 );
//     //OUT_SetState( OUT_GetPtr(OUT_ID_1), expResult);
//     OUT_ToggleState( OUT_GetPtr(OUT_ID_1) );
//     osDelay(3000);
//   }
// }