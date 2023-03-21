#include <memory.h>
#include "vnf1048.h"

#define READ_MASK 0b01000000
#define READ_ROM_MASK 0b11000000
#define DEBUG 1

uint8_t get_bit(uint8_t byte, uint8_t index)
{
    return (byte >> index) & 1;
}

VNF_ErrorTypeDef vnf_check_global_status(volatile uint8_t glb)
{
    if(get_bit(glb, 5))
    {
        return VNF_SPIE;
    }
}

/**
  * @brief Initializes communication with VNF1048.
  * @param handle handle used with VNF1048
  * @retval None
  */
void vnf_init(VNF1048_HandleTypeDef* handle)
{
    /* To go from STAND-BY to UNLOCKED or LOCKED */
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    /* Can be as low as 100us */
    HAL_Delay(10);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);
    handle->locked = 0x1;
}

/**
  * @brief Reads VNF1048 ram register.
  * @param reg register address
  * @param res ptr for result memory
  * @retval SPI-HAL status
  */
VNF_StatusTypeDef vnf_read_reg(VNF1048_HandleTypeDef* handle, const uint8_t reg, uint8_t res[4])
{
    VNF_StatusTypeDef status = VNF_OK;
    HAL_StatusTypeDef hal_status = HAL_OK;
    uint8_t tx[4] = { reg | READ_MASK, 0x00, 0x00, 0x00};

    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    hal_status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);

    /* Set error register for current error */
    handle->error_register = vnf_check_global_status(res[0]);
    if(hal_status != HAL_OK)
        handle->error_register |= VNF_HAL_ERROR;
    if(handle->error_register != 0x00)
        status = VNF_ERROR;

#ifdef DEBUG
    printf("R: 0x%x SPI: %x %x %x %x\n", reg, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

VNF_StatusTypeDef vnf_read_rom(VNF1048_HandleTypeDef* handle, uint8_t addr, uint8_t res[4])
{
    VNF_StatusTypeDef status = VNF_OK;
    HAL_StatusTypeDef  hal_status = HAL_OK;
    uint8_t tx[4] = { addr | READ_ROM_MASK, 0x00, 0x00, 0x00};

    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    hal_status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);

    /* Set error register for current error */
    handle->error_register = vnf_check_global_status(res[0]);
    if(hal_status != HAL_OK)
        handle->error_register |= VNF_HAL_ERROR;
    if(handle->error_register != 0x00)
        status = VNF_ERROR;

    /* Due to datasheet rx[2] and rx[3] should be 0x00 */
#ifdef DEBUG
    printf("ROM: 0x%x SPI: %x %x %x %x\n", addr, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

VNF_StatusTypeDef vnf_write_reg(VNF1048_HandleTypeDef* handle, const uint8_t reg, uint8_t data[3], uint8_t res[4])
{
    VNF_StatusTypeDef status = VNF_OK;
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    /* Check if valid write register */
    if(reg < VNF_CONTROL_REGISTER_1
        || reg > VNF_CONTROL_REGISTER_3)
    {
#ifdef DEBUG
        printf("Invalid write register 0x%x\n", reg);
#endif
        return status;
    }

    /* For write there is no mask necessary */
    uint8_t tx[4] = { reg, 0x00, 0x00, 0x00};
    memcpy(&tx[1], data, 3);

    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    hal_status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);

    /* Set error register for current error */
    handle->error_register = vnf_check_global_status(res[0]);
    if(hal_status != HAL_OK)
        handle->error_register |= VNF_HAL_ERROR;
    if(handle->error_register != 0x00)
        status = VNF_ERROR;

    return status;
}

void vnf_unlock(VNF1048_HandleTypeDef* handle)
{
    /* TODO here add unlock method */
}
