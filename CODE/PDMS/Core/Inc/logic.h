#ifndef __LOGIC_H_
#define __LOGIC_H_

#include "typedefs.h"

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
} T_LOGIC_OPERATOR;

typedef enum
{
  // REMEMBER SCHMITT ALSO ALLOWS CONST (TRUE / FALSE )
  LOGIC_VAR_SCHMITT = 0x00,
  LOGIC_VAR_ANALOG = 0x01,
  LOGIC_VAR_ANY = 0x02,
  LOGIC_VAR_NONE = 0x03
} T_LOGIC_VAR;

typedef enum
{
    LOGIC_INPUT_TYPE_SENSOR = 0x00,
    LOGIC_INPUT_TYPE_CONST  = 0x01
}T_LOGIC_INPUT_TYPE;

/* TODO At home 
    1. DONE Change below T_LOGIC_VAR into T_LOGIC_INPUT_TYPE
    2. DONE Change var1data and var2data into input id or const value
    3. operator ok
    4. Change expression validation that checks input configuration (or const)
    5. Change expression evaluation so that it reads input data specific to configuration analog or digital
    6. Evaluate value
*/


/// @brief Logic expression
typedef struct
{
  T_LOGIC_INPUT_TYPE var1_type;
  union
  {
    uint32_t   var1_const;
    T_INPUT_ID var1_data;
  };

  T_LOGIC_INPUT_TYPE var2_type;
    union
  {
    uint32_t   var2_const;
    T_INPUT_ID var2_data;
  };

  T_LOGIC_OPERATOR opr;
} T_LOGIC_EXPRESSION;

typedef struct
{
  bool isUsed;
  T_LOGIC_EXPRESSION exp;
  bool result;
} T_LOGIC;

extern T_LOGIC logics[POWER_OUT_COUNT];

#endif