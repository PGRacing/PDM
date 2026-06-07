#ifndef __BSP_PHYINPUT_H_
#define __BSP_PHYINPUT_H_

#include "typedefs.h"
#include "bsp_cmninput.h"

typedef enum
{
    IN_PHY_LOC_1 = 0x00,
    IN_PHY_LOC_2 = 0x01,
    IN_PHY_LOC_3 = 0x02,
    IN_PHY_LOC_4 = 0x03,
    IN_PHY_LOC_5 = 0x04,
    IN_PHY_LOC_6 = 0x05,
    IN_PHY_LOC_7 = 0x06,
    IN_PHY_LOC_8 = 0x07,
    IN_PHY_MAX
}T_IN_PHY_LOC;

/// @brief This function perfroms work on data from inputs should be called after input data acquisition
/// @param  None
void BSP_PHYIN_EvaluateValues(void);

/// @brief Get physical input value as schmitt trigger
/// @param location location of the physical input
/// @return Input value as schmitt trigger
/// @note This function should be called only for physical inputs
bool BSP_PHYIN_GetValueSchmitt(T_IN_PHY_LOC location);

/// @brief Get physical input value as analog
/// @param location location of the physical input
/// @return Input value as analog
/// @note This function should be called only for physical inputs
uint_fast16_t BSP_PHYIN_GetValueAnalog(T_IN_PHY_LOC location);

#endif