#include "main.h"
#include "stm32l4xx_hal_spi.h"

/* SPI handle */
extern SPI_HandleTypeDef*  hspi_vnf;

/** \defgroup VNF_REGISTERS
 *  @{
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
/** @}*/


#define VNF_ADVANCED_OPERATION 0x3F

void vnf_init(SPI_HandleTypeDef* hspi_vnf_in, GPIO_TypeDef* CS_Port, uint16_t CS_Pin);
HAL_StatusTypeDef vnf_read_reg(uint8_t reg, uint8_t res[4]);
HAL_StatusTypeDef vnf_write_reg(uint8_t reg, uint8_t data[3], uint8_t res[4]);
