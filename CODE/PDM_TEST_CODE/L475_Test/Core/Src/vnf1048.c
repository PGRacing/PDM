#include <memory.h>
#include "vnf1048.h"

#define READ_MASK 0b01000000
#define READ_ROM_MASK 0b11000000
//#define VNF_DEBUG 1

uint8_t bit_manip_get(uint8_t byte, uint8_t index)
{
    return (byte >> index) & 1;
}

uint8_t bit_manip_set(uint8_t* byte, uint8_t index)
{
    return *byte |= 1 << index;
}

uint8_t bit_manip_reset(uint8_t* byte, uint8_t index)
{
    return *byte &= ~(1 << index);
}

VNF_ErrorTypeDef vnf_check_global_status(volatile uint8_t glb)
{
    if(bit_manip_get(glb, 5))
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
    /* Setting HWLO bit */
    HAL_GPIO_WritePin(handle->HWLO_Port, handle->HWLO_Pin, GPIO_PIN_RESET);

    /* Starting watchdog */
    //HAL_TIM_Base_Start_IT(handle->wd_tim);

    /* To go from STAND-BY to UNLOCKED or LOCKED */
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    /* Can be as low as 100us */
    HAL_Delay(0.5);
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
    uint8_t tx[4] = { reg | READ_MASK, 0x69, 0x69, 0x69};

    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    hal_status = HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);

    /* Set error register for current error */
    handle->error_register = vnf_check_global_status(res[0]);
    if(hal_status != HAL_OK)
        handle->error_register |= VNF_HAL_ERROR;
    if(handle->error_register != 0x00)
        status = VNF_ERROR;

#ifdef VNF_DEBUG
    printf("READ-REG-OP REG: 0x%x SPI: %x %x %x %x\n", reg, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

VNF_StatusTypeDef vnf_read_rom(VNF1048_HandleTypeDef* handle, uint8_t addr, uint8_t res[4])
{
    VNF_StatusTypeDef status = VNF_OK;
    HAL_StatusTypeDef  hal_status = HAL_OK;
    uint8_t tx[4] = { addr | READ_ROM_MASK, 0x55, 0x55, 0x55};

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
#ifdef VNF_DEBUG
    printf("READ-ROM-OP ADDR: 0x%x SPI: %x %x %x %x\n", addr, res[0], res[1], res[2], res[3]);
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
#ifdef VNF_DEBUG
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

#ifdef VNF_DEBUG
    //printf("TOBEWRITTEN REG: 0x%x SPI: %x %x %x %x\n", reg, tx[0], tx[1], tx[2], tx[3]);
    //printf("WRITE-OP REG: 0x%x SPI: %x %x %x %x\n", reg, res[0], res[1], res[2], res[3]);
#endif

    return status;
}

void vnf_unlock(VNF1048_HandleTypeDef* handle)
{
    uint8_t cr3[4] = { 0x00, 0x00, 0x00, 0x00};
    //vnf_read_reg(handle, VNF_CONTROL_REGISTER_3, cr3);
    uint8_t cr1[4] = { 0x00, 0x00, 0x00, 0x00};
    //vnf_read_reg(handle, VNF_CONTROL_REGISTER_1, cr1);

    /* Set unlock bit */
    bit_manip_set(&(cr3[2]), 1);
    bit_manip_reset(&(cr1[2]),3);
    bit_manip_set(&(cr1[2]),2);

    uint8_t rx[4] = { 0x00, 0x00, 0x00, 0x00};
    vnf_write_reg(handle, VNF_CONTROL_REGISTER_3, &(cr3[1]), rx);
    vnf_write_reg(handle, VNF_CONTROL_REGISTER_1, &(cr1[1]), rx);

    //HAL_Delay(100);
    vnf_read_reg(handle, VNF_CONTROL_REGISTER_1, rx);
}

void vnf_toggle_wdg(VNF1048_HandleTypeDef* handle)
{
#ifdef VNF_DEBUG
    printf("TGL\n");
#endif

    uint8_t cr3[4] = { 0x00, 0x00, 0x00, 0x00};
    vnf_read_reg(handle,VNF_CONTROL_REGISTER_3, cr3);

    if(bit_manip_get(cr3[3], 1))
    {
        bit_manip_reset(&(cr3[3]), 1);
    }else
    {
        bit_manip_set(&(cr3[3]), 1);
    }

    uint8_t rx[4] = { 0x00, 0x00, 0x00, 0x00};
    vnf_write_reg(handle, VNF_CONTROL_REGISTER_3, &(cr3[1]), rx);
}

void vnf_status1_read_error(VNF1048_HandleTypeDef* handle, uint8_t data[4])
{
    printf("WD FAIL: %d", bit_manip_get(data[1], VNF_ST1_WD_FAIL));
}

void vnf_helper_print_data(uint8_t addr, uint8_t res[4])
{
    printf("PRINT: 0x%x SPI: 0x%x 0x%x 0x%x 0x%x\n", addr, res[0], res[1], res[2], res[3]);
}

void vnf_special_SWReset(VNF1048_HandleTypeDef* handle)
{
    uint8_t tx[4] = {0xFF, 0x00, 0x00, 0x00};
    uint8_t res[4] = {0x00, 0x00, 0x00, 0x00};
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);
}

void vnf_special_clear_status(VNF1048_HandleTypeDef* handle)
{
    uint8_t tx[4] = {0xBF, 0x00, 0x00, 0x00};
    uint8_t res[4] = {0x00, 0x00, 0x00, 0x00};
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(handle->hspi_vnf, tx, res, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(handle->CS_Port, handle->CS_Pin, GPIO_PIN_SET);
}