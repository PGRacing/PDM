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
  LOGIC_VAR_TYPE_SCHMITT = 0x00,
  LOGIC_VAR_TYPE_ANALOG = 0x01,
  LOGIC_VAR_TYPE_ANY = 0x02,
  LOGIC_VAR_TYPE_NONE = 0x03
} T_LOGIC_VAR_TYPE;

typedef enum
{
    LOGIC_INPUT_TYPE_SENSOR = 0x00,
    LOGIC_INPUT_TYPE_CONST_SCHMITT = 0x01,
    LOGIC_INPUT_TYPE_CONST_ANALOG = 0x02,
    LOGIC_INPUT_TYPE_UNSET = 0x03
}T_LOGIC_INPUT_TYPE;


/// @brief Logic expression
typedef struct
{
  T_LOGIC_INPUT_TYPE input1Type;
  union
  {
    uint32_t   input1Const;
    T_INPUT_ID input1ID;
  };

  T_LOGIC_INPUT_TYPE input2Type;
  union
  {
    uint32_t   input2Const;
    T_INPUT_ID input2ID;
  };

  T_LOGIC_OPERATOR opr;
} T_LOGIC_EXPRESSION;

typedef struct
{
  bool isUsed;
  T_LOGIC_EXPRESSION exp;
} T_LOGIC;

//extern bool logicResults[POWER_OUT_COUNT];

/// @brief Evaluate logic expressions controling all outputs
/// @return Array of evaluated results
bool* LOGIC_Evaluate();

#endif