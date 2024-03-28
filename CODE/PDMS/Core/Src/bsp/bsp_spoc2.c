#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"
#include "stm32l4xx_ll_tim.h"
#include "typedefs.h"
#include "logger.h"
#include "bsp_spoc2.h"
#include "spi.h"

typedef struct _T_BSP_SPOC2_CFG
{
    SPI_HandleTypeDef* phspi;
    T_IO csnIo;
    T_IO adcChannelIo;
}T_BSP_SPOC2_CFG;

// Configuration of on-board SPOC-2 (whole) devices
static T_BSP_SPOC2_CFG bspSpoc2Cfg[SPOC2_NUM_OF_DEVICES] = 
{
    {
        .phspi = &hspi2,
        .csnIo = {.port = LP_CSN1_GPIO_Port, .pin = LP_CSN1_Pin },
        .adcChannelIo = {.port = SENS_OUT_LP1_GPIO_Port, SENS_OUT_LP1_Pin}
    },
    {
        .phspi = &hspi2,
        .csnIo = {.port = LP_CSN2_GPIO_Port, .pin = LP_CSN2_Pin },
        .adcChannelIo = {.port = SENS_OUT_LP2_GPIO_Port, SENS_OUT_LP2_Pin}
    },
};

static void BSP_SPOC2_SlaveSelect(T_SPOC2_ID id)
{   
    ASSERT( id < SPOC2_ID_MAX);

    LL_GPIO_ResetOutputPin(bspSpoc2Cfg[id].csnIo.port, bspSpoc2Cfg[id].csnIo.pin);
}

static void BSP_SPOC2_SlaveDeselect(T_SPOC2_ID id)
{
    ASSERT( id < SPOC2_ID_MAX);

    LL_GPIO_SetOutputPin(bspSpoc2Cfg[id].csnIo.port, bspSpoc2Cfg[id].csnIo.pin);
}   

bool BSP_SPOC2_TransferBlocking(T_SPOC2_ID id, uint8* txBuffer, uint8* rxBuffer, uint32 dataLen)
{
    ASSERT( id < SPOC2_ID_MAX);

    HAL_StatusTypeDef result = HAL_OK;
    // Transfer data over selected spi interface in blocking mode (requiered by spoc2 library)
    BSP_SPOC2_SlaveSelect(id);

    if( txBuffer != NULL)
    {
        result = HAL_SPI_Transmit(bspSpoc2Cfg[id].phspi, txBuffer, dataLen, HAL_MAX_DELAY);
    }
    
    if( rxBuffer != NULL)
    {
        result |= HAL_SPI_Receive(bspSpoc2Cfg[id].phspi, rxBuffer, dataLen, HAL_MAX_DELAY);
    }
    
    BSP_SPOC2_SlaveDeselect(id);

    if( result != HAL_OK)
    {
        return false;
    }

    return true;
}