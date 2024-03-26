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


void BSP_SPOC2_SlaveSelect()
{
    // WARN Using both slave select same time and daisy chainging for easier outcome
    LL_GPIO_SetOutputPin(LP_CSN1_GPIO_Port, LP_CSN1_Pin);
    LL_GPIO_SetOutputPin(LP_CSN2_GPIO_Port, LP_CSN2_Pin);
}

void BSP_SPOC2_SlaveDeselect()
{
    // WARN Using both slave select same time and daisy chainging for easier outcome
    LL_GPIO_ResetOutputPin(LP_CSN1_GPIO_Port, LP_CSN1_Pin);
    LL_GPIO_ResetOutputPin(LP_CSN2_GPIO_Port, LP_CSN2_Pin);
}   


// FIXME Here function definitions change naming convention due to the fact that external driver is used

SPOC2_error_t SPOC2_SPI_writeBuffer(uint32 spiBusId, uint8 spiChipSelect, uint8* txBuffer, uint32 dataLen) 
{
    BSP_SPOC2_SlaveSelect();
    HAL_SPI_Transmit(&hspi2, txBuffer, dataLen, HAL_MAX_DELAY);
    // these API functions cannot fail
    
    return SPOC2_ERROR_OK;
}

// function to read data from the SPOC+2 out of the SPI receive buffer
SPOC2_error_t SPOC2_SPI_readRxBuffer(uint32 spiBusId, uint8* rxBuffer, uint32 dataLen) 
{

    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_SPI_Receive(&hspi2, rxBuffer, dataLen, HAL_MAX_DELAY);
    
    BSP_SPOC2_SlaveDeselect();

    return (status == HAL_OK) ? SPOC2_ERROR_OK : SPOC2_ERROR_RX_FAILURE;
}

// function to clear the internal RX FIFO of unread data
void SPOC2_SPI_clearRxBuffer(uint32 spiBusId) 
{ 
    //HAL_SPI_FlushRxFifo(&hspi2);
    __NOP();
}

// function to indicate whether the previous SPI transfer is complete
boolean SPOC2_SPI_isTransferComplete(uint32 spiBusId) 
{ 
    return true; 
}