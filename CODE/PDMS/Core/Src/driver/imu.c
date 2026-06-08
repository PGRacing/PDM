#include "asm330.h"
#include "imu.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "freertos_os2.h"
#include "cmsis_os2.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>
#include "gpio.h"

#define SENSOR_BUS hi2c2

#define BOOT_TIME 200 //ms

// IMU is rotated by 45 degrees regarding PDM caseing, the output will be corrected
// IMU direction X is in direction of device header 
#define IMU_IC_ROTATION_FACTOR_SIN_THETA 0.70710678118
#define IMU_IC_ROTATION_FACTOR_COS_THETA 0.70710678118

static int32_t IMU_PlatformWrite(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t IMU_PlatfromRead(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);

static void IMU_PlatfromDelay(uint32_t ms);

typedef struct _T_IMU_HANDLE
{
    stmdev_ctx_t devCtx;
    int16_t dataRawAcc[3];
    int16_t dataRawAngRate[3];
    int16_t dataRawTemperature;
    float_t temperatureDegC; // *C
    uint8_t whoamI;
    T_IMU_DATA_ACC acceleration;
    T_IMU_DATA_ACC correctedAcc; // Acceleration regarding PDM caseing
    T_IMU_DATA_RATE rate;
}
T_IMU_HANDLE;

static T_IMU_HANDLE imuHandle;

/// @brief Configure IMU
/// @param handle IMU handle
static void IMU_Configure(T_IMU_HANDLE* handle)
{
    uint8_t rst = 0;

    // Initialize mems driver interface
    handle->devCtx.write_reg = IMU_PlatformWrite;
    handle->devCtx.read_reg = IMU_PlatfromRead;
    handle->devCtx.mdelay = IMU_PlatfromDelay;
    handle->devCtx.handle = &SENSOR_BUS;


    // Wait sensor boot time
    IMU_PlatfromDelay(BOOT_TIME);
    // Check device ID
    asm330lhh_device_id_get(&(handle->devCtx), &(handle->whoamI));

    if (handle->whoamI != ASM330LHH_ID)
    {
    LOG_ERR("IMU:: Device not found");
    for(;;)
    {
        osDelay(pdMS_TO_TICKS(1000));
    }
    }

    // Restore default configuration
    asm330lhh_reset_set(&(handle->devCtx), PROPERTY_ENABLE);

    do {
        asm330lhh_reset_get(&(handle->devCtx), &rst);
        osDelay(1);
    } while (rst);

    // Start device configuration.
    asm330lhh_device_conf_set(&(handle->devCtx), PROPERTY_ENABLE);
    // Enable Block Data Update
    asm330lhh_block_data_update_set(&(handle->devCtx), PROPERTY_ENABLE);
    // Set Output Data Rate
    asm330lhh_xl_data_rate_set(&(handle->devCtx), ASM330LHH_XL_ODR_52Hz);
    asm330lhh_gy_data_rate_set(&(handle->devCtx), ASM330LHH_XL_ODR_52Hz);
    // Set full scale
    asm330lhh_xl_full_scale_set(&(handle->devCtx), ASM330LHH_8g);
    asm330lhh_gy_full_scale_set(&(handle->devCtx), ASM330LHH_2000dps);

    // Configure filtering chain(No aux interface)
    //  * Accelerometer - LPF1 + LPF2 path

    asm330lhh_xl_hp_path_on_out_set(&(handle->devCtx), ASM330LHH_LP_ODR_DIV_100);
    asm330lhh_xl_filter_lp2_set(&(handle->devCtx), PROPERTY_ENABLE);
}

/// @brief Get data from IMU via polling
/// @param handle IMU handle
void IMU_ReadDataPolling(T_IMU_HANDLE* handle)
{
  // Read samples in polling mode (no int)
  uint8_t reg;
  // Read output only if new xl value is available
  asm330lhh_xl_flag_data_ready_get(&(handle->devCtx), &reg);

  if (reg) {
    // Read acceleration field data
    memset(handle->dataRawAcc, 0x00, 3 * sizeof(int16_t));
    asm330lhh_acceleration_raw_get(&(handle->devCtx), handle->dataRawAcc);
    handle->acceleration.x = asm330lhh_from_fs8g_to_mg(handle->dataRawAcc[0]);
    handle->acceleration.y = asm330lhh_from_fs8g_to_mg(handle->dataRawAcc[1]);
    handle->acceleration.z = asm330lhh_from_fs8g_to_mg(handle->dataRawAcc[2]);

    handle->correctedAcc.x = handle->acceleration.x * IMU_IC_ROTATION_FACTOR_COS_THETA - 
                                handle->acceleration.y * IMU_IC_ROTATION_FACTOR_SIN_THETA;

    handle->correctedAcc.y = handle->acceleration.x * IMU_IC_ROTATION_FACTOR_SIN_THETA + 
                                handle->acceleration.y * IMU_IC_ROTATION_FACTOR_COS_THETA;

    handle->correctedAcc.z = handle->acceleration.z;
  }

  asm330lhh_gy_flag_data_ready_get(&(handle->devCtx), &reg);

  if (reg) {
    // Read angular rate field data
    memset(handle->dataRawAngRate, 0x00, 3 * sizeof(int16_t));
    asm330lhh_angular_rate_raw_get(&(handle->devCtx), handle->dataRawAngRate);
    handle->rate.pitch = asm330lhh_from_fs2000dps_to_mdps(handle->dataRawAngRate[0]);
    handle->rate.roll  = asm330lhh_from_fs2000dps_to_mdps(handle->dataRawAngRate[1]);
    handle->rate.yaw   = asm330lhh_from_fs2000dps_to_mdps(handle->dataRawAngRate[2]);
  }

  asm330lhh_temp_flag_data_ready_get(&(handle->devCtx), &reg);

  if (reg) {
    // Read temperature data
    memset(&(handle->dataRawTemperature), 0x00, sizeof(int16_t));
    asm330lhh_temperature_raw_get(&(handle->devCtx), &(handle->dataRawTemperature));
    handle->temperatureDegC = asm330lhh_from_lsb_to_celsius(handle->dataRawTemperature);
  }
}

/// @brief Write generic device register (platform dependent)
/// @param handle Customizable argument used to select the correct sensor bus handler
/// @param reg Register to write
/// @param bufp Pointer to data to write in register reg
/// @param len Number of consecutive registers to write
static int32_t IMU_PlatformWrite(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
    HAL_I2C_Mem_Write(handle, ASM330LHH_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
    return 0;
}

/// @brief Read generic device register (platform dependent)
/// @param handle Customizable argument used to select the correct sensor bus handler
/// @param reg Register to read
/// @param bufp Pointer to buffer that stores the data read
/// @param len Number of consecutive registers to read
static int32_t IMU_PlatfromRead(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
    HAL_I2C_Mem_Read(handle, ASM330LHH_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
    return 0;
}

/// @brief Platform dependent delay
/// @param ms
static void IMU_PlatfromDelay(uint32_t ms)
{
    osDelay(pdMS_TO_TICKS(ms));
}

/// @brief Initialize platform for IMU
/// @param
void IMU_Init(void)
{
    // Set I2C mode
    LL_GPIO_SetOutputPin(IMU_MODE_SEL_GPIO_Port, IMU_MODE_SEL_Pin);

    // Set SA0 pin to high (ASM330LHH_I2C_ADD_H)
    LL_GPIO_SetOutputPin(IMU_SA0_GPIO_Port, IMU_SA0_Pin);
}

/// @brief Get acceleration data from IMU
/// @param acc [in] Pointer to structure where acceleration data will be returned [mg]
void IMU_GetAcceleration(T_IMU_DATA_ACC* acc)
{
    memcpy(acc, &(imuHandle.correctedAcc), sizeof(T_IMU_DATA_ACC));
}

/// @brief Get gyro data from IMU
/// @param acc [in] Pointer to structure where gyro data will be returned [mdps]
void IMU_GetRates(T_IMU_DATA_RATE* rate)
{
    memcpy(rate, &(imuHandle.rate), sizeof(T_IMU_DATA_RATE));
}

/// @brief Get temperature data from IMU
/// @returns temperature data in celcius degrees
float_t IMU_GetTemperature(void)
{
    return imuHandle.temperatureDegC;
}

#ifdef DEBUG
volatile static uint32_t DWTimuTS = 0;
volatile static uint32_t DWTimuPROC = 0;
#endif

void imuTaskEntry(void *argument)
{
    LOG_INFO("IMU:: Task start");

    IMU_Configure(&imuHandle);

    for(;;)
    {
    #ifdef DEBUG
        DWTimuTS = DEBUG_ARM_GET_TIME; 
    #endif

        IMU_ReadDataPolling(&imuHandle);

    #ifdef DEBUG
        DWTimuPROC = DEBUG_ARM_CLOCKS_TO_US(DEBUG_ARM_GET_TIME - DWTimuTS);
    #endif

        osDelay(pdMS_TO_TICKS(25));
    }
}