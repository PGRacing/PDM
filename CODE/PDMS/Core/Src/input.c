#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "input.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "adc.h"
#include "cmsis_os2.h"

// Physical inputs shouldn't be redefined
T_IN_CFG inputsCfg[] =
    {
        [0] =
            {
                .id = IN_PHY_ID_1,
                .io = {IO_CONN1_GPIO_Port, IO_CONN1_Pin},
                .rawData = &(adc2RawData[0]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [1] =
            {
                .id = IN_PHY_ID_2,
                .io = {IO_CONN2_GPIO_Port, IO_CONN2_Pin},
                .rawData = &(adc2RawData[1]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [2] =
            {
                .id = IN_PHY_ID_3,
                .io = {IO_CONN3_GPIO_Port, IO_CONN3_Pin},
                .rawData = &(adc2RawData[2]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [3] =
            {
                .id = IN_PHY_ID_4,
                .io = {IO_CONN4_GPIO_Port, IO_CONN4_Pin},
                .rawData = &(adc2RawData[3]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [4] =
            {
                .id = IN_PHY_ID_5,
                .io = {IO_CONN5_GPIO_Port, IO_CONN5_Pin},
                .rawData = &(adc2RawData[4]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [5] =
            {
                .id = IN_PHY_ID_6,
                .io = {IO_CONN6_GPIO_Port, IO_CONN6_Pin},
                .rawData = &(adc2RawData[5]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [6] =
            {
                .id = IN_PHY_ID_7,
                .io = {IO_CONN7_GPIO_Port, IO_CONN7_Pin},
                .rawData = &(adc2RawData[6]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
        [7] =
            {
                .id = IN_PHY_ID_8,
                .io = {IO_CONN8_GPIO_Port, IO_CONN8_Pin},
                .rawData = &(adc2RawData[7]),
                .type = IN_TYPE_PHY,
                .mode = IN_MODE_UNUSED},
};

static T_IN_CFG *IN_GetPtr(uint8_t id)
{
  // TODO Extend for can inputs
  ASSERT(id > 0 && id < ARRAY_COUNT(inputsCfg));
  return &(inputsCfg[id]);
}

bool IN_IsPhysical(T_IN_CFG* cfg)
{
  ASSERT( cfg );
  ASSERT( cfg->id > 0 && cfg->id < ARRAY_COUNT(inputsCfg) );

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
  ASSERT( cfg->id > 0 && cfg->id < ARRAY_COUNT(inputsCfg) );

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