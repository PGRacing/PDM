#include <memory.h>
#include "vnf1048.h"

#define READ_MASK 0b01000000
#define DEBUG 1

SPI_HandleTypeDef*  hspi_vnf;
GPIO_TypeDef* vnf_cs_port;
uint16_t vnf_cs_pin;

/**
  * @brief Initializes communication with VNF1048.
  * @param hspi_vnf_in spi handle used with VNF1048
  * @param CS_Port chip select port
  * @param CS_Pin chip select pin
  * @retval None
  */
void vnf_init(SPI_HandleTypeDef* hspi_vnf_in, GPIO_TypeDef* CS_Port, uint16_t CS_Pin)
{
    hspi_vnf = hspi_vnf_in;
    vnf_cs_port = CS_Port;
    vnf_cs_pin = CS_Pin;

    /* To go from STAND-BY to UNLOCKED or LOCKED */
    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_RESET);
    /* Can be as low as 100us */
    HAL_Delay(10);
    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_SET);
}

/**
  * @brief Reads VNF1048 ram register.
  * @param reg register address
  * @param res ptr for result memory
  * @retval SPI-HAL status
  */
HAL_StatusTypeDef vnf_read_reg(const uint8_t reg, uint8_t res[4])
{
    HAL_StatusTypeDef status;
    uint8_t tx[4] = { reg | READ_MASK, 0x00, 0x00, 0x00};

    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_SET);

#ifdef DEBUG
    printf("R: %x SPI: %x %x %x %x\n", reg, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

HAL_StatusTypeDef vnf_write_reg(uint8_t reg, uint8_t data[3], uint8_t res[4])
{
    HAL_StatusTypeDef status;
    /* For write there is no mask necessary */
    uint8_t tx[4] = { reg, 0x00, 0x00, 0x00};
    memcpy(&tx[1], data, 3);

    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(vnf_cs_port, vnf_cs_pin, GPIO_PIN_SET);

    return status;
}