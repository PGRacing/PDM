#include "main.h"
#include "stm32l4xx_hal_spi.h"

/**
  * @brief VNF Status structures definition
  */
typedef enum
{
    VNF_OK       = 0x00,
    VNF_ERROR    = 0xAB,
} VNF_StatusTypeDef;

/**
  * @brief VNF error structures definition
  */
typedef enum
{
    VNF_NOERROR       = 0x00,
    VNF_SPIE    = 0x01,
    VNF_HAL_ERROR = 0x02,
    VNF_NEXT_ERROR = 0x04
} VNF_ErrorTypeDef;

/**
  * @brief VNF1048 Handle
  */
typedef struct VNF1048_HandleTypeDef_A
{
    SPI_HandleTypeDef*  hspi_vnf;     /*!< SPI Handle */
    GPIO_TypeDef* CS_Port;            /*!< SPI chip select port */
    uint16_t CS_Pin;                  /*!< SPI chip select pin */
    GPIO_TypeDef* HWLO_Port;          /*!< HWLO port */
    uint16_t HWLO_Pin;                /*!< HWLO pin */
    uint8_t locked;                   /*!< IC state */
    uint8_t error_register;   /*!< ERROR state */
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


/** \defgroup VNF_ROM
 * ///@{
 */
#define VNF_ROM_COMPANY_CODE 0x00
#define VNF_ROM_DEVICE_FAMILY 0x01
#define VNF_ROM_PRODUCT_CODE_1 0x02
#define VNF_ROM_PRODUCT_CODE_2 0x03
#define VNF_ROM_PRODUCT_CODE_3 0x04
#define VNF_ROM_PRODUCT_CODE_4 0x05
#define VNF_ROM_SILICON_VERSION 0x0A
#define VNF_ROM_SPI_MODE 0x10
#define VNF_ROM_WD_TYPE_1 0x11
#define VNF_ROM_WD_BIT_POS_1 0x13
#define VNF_ROM_WD_BIT_POS_2 0x14
#define VNF_ADVANCED_OPERATION 0x3F
/** ///@}*/


void vnf_init(VNF1048_HandleTypeDef* handle);
VNF_StatusTypeDef vnf_read_reg(VNF1048_HandleTypeDef* handle, uint8_t reg, uint8_t res[4]);
VNF_StatusTypeDef vnf_read_rom(VNF1048_HandleTypeDef* handle, uint8_t addr, uint8_t res[4]);
VNF_StatusTypeDef vnf_write_reg(VNF1048_HandleTypeDef* handle, uint8_t reg, uint8_t data[3], uint8_t res[4]);
void vnf_unlock(VNF1048_HandleTypeDef* handle);

uint8_t bit_manip_set(uint8_t* byte, uint8_t index);
uint8_t bit_manip_reset(uint8_t* byte, uint8_t index);
uint8_t bit_manip_get(uint8_t byte, uint8_t index);