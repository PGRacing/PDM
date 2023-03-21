#include <memory.h>
#include "vnf1048.h"

#define READ_MASK 0b01000000
#define DEBUG 1

uint8_t get_bit(uint8_t byte, uint8_t index)
{
    return (byte >> index) & 1;
}

uint8_t check_global_status(volatile uint8_t glb)
{
    if(get_bit(glb, 5))
    {
        printf("SPI ERROR!\n");
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
HAL_StatusTypeDef vnf_read_reg(VNF1048_HandleTypeDef* handle, const uint8_t reg, uint8_t res[4])
{
    HAL_StatusTypeDef status;
    uint8_t tx[4] = { reg | READ_MASK, 0x00, 0x00, 0x00};

    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);

    printf("\n");
    check_global_status(res[0]);
#ifdef DEBUG
    printf("R: 0x%x SPI: %x %x %x %x\n", reg, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

HAL_StatusTypeDef vnf_write_reg(VNF1048_HandleTypeDef* handle, const uint8_t reg, uint8_t data[3], uint8_t res[4])
{
    HAL_StatusTypeDef status = HAL_ERROR;
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
    status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);
    check_global_status(res[0]);

    return status;
}

void vnf_unlock(VNF1048_HandleTypeDef* handle)
{

}
