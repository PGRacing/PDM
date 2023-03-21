#include "main.h"
#include "stm32l4xx_hal_spi.h"

/**
  * @brief VNF1048 Handle
  */
typedef struct VNF1048_HandleTypeDef_A
{
    SPI_HandleTypeDef*  hspi_vnf;     /*!< SPI Handle */
    GPIO_TypeDef* CS_Port;  /*!< SPI chip select port */
    uint16_t CS_Pin; /*!< SPI chip select pin */
    uint8_t locked; /*!< IC state */
} VNF1048_HandleTypeDef;

/* TODO fix documentation of defines */

/** \defgroup VNF_REGISTERS
 * ///@{
 */
/* Control registers */
/* CONTROLS */
#define VNF_CONTROL_REGISTER_1 0x01
/* CONFIG 1 */
#define VNF_CONTROL_REGISTER_2 0x02
/* CONFIG 3 */
#define VNF_CONTROL_REGISTER_3 0x03

/* Status registers */
#define VNF_STATUS_REGISTER_1 0x11
#define VNF_STATUS_REGISTER_2 0x12
#define VNF_STATUS_REGISTER_3 0x13
#define VNF_STATUS_REGISTER_4 0x14
#define VNF_STATUS_REGISTER_5 0x15
#define VNF_STATUS_REGISTER_6 0x16
#define VNF_STATUS_REGISTER_7 0x17
#define VNF_STATUS_REGISTER_8 0x18
/** ///@}*/

#define VNF_ADVANCED_OPERATION 0x3F

void vnf_init(VNF1048_HandleTypeDef* handle);
HAL_StatusTypeDef vnf_read_reg(VNF1048_HandleTypeDef* handle, uint8_t reg, uint8_t res[4]);
HAL_StatusTypeDef vnf_write_reg(VNF1048_HandleTypeDef* handle, uint8_t reg, uint8_t data[3], uint8_t res[4]);
void vnf_unlock(VNF1048_HandleTypeDef* handle);
