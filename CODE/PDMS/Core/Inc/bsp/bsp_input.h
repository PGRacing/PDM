#ifndef __BSP_INPUT_H_
#define __BSP_INPUT_H_

#include "typedefs.h"

typedef enum
{
    IN_PHY_ID_1 = 0x00,
    IN_PHY_ID_2 = 0x01,
    IN_PHY_ID_3 = 0x02,
    IN_PHY_ID_4 = 0x03,
    IN_PHY_ID_5 = 0x04,
    IN_PHY_ID_6 = 0x05,
    IN_PHY_ID_7 = 0x06,
    IN_PHY_ID_8 = 0x07,
    IN_PHY_MAX
}T_IN_PHY_ID;

typedef struct _T_BSP_IN_CFG
{
    const T_INPUT_ID  id;      // id should reflect position in inputsCfg
    const T_IO        io;
    uint16_t * const  rawData; // pointer to rawData from input
}T_BSP_IN_CFG;

typedef struct _T_BSP_IN_REG
{
   uint_fast16_t analogValue;
   bool          schmittState;
}
T_BSP_IN_REG;

/// @brief This function perfroms work on data from inputs should be called in main slow loop   
/// @param 
void BSP_IN_EvaluateValues(void);

/// @brief Get physical input value as schmitt trigger
/// @param id Physical input ID
/// @return Input value as shcmitt trigger
/// @note This function should be called only for physical inputs
bool BSP_IN_GetValueSchmitt(T_INPUT_ID id);

/// @brief Get physical input value as analog
/// @param id Physical input ID
/// @return Input value as analog
/// @note This function should be called only for physical inputs
uint_fast16_t BSP_IN_GetValueAnalog(T_INPUT_ID id);

#endif