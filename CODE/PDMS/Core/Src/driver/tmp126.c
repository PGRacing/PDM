#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_gpio.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "tmp126.h"
#include "logger.h"
#include "gpio.h"
#include "freertos_os2.h"

#define TMP126_CSPort TEMP1_CS_GPIO_Port
#define TMP126_CSPin TEMP1_CS_Pin
#define TMP126_SPI_HANDLE hspi1

#define TMP126_READ_MASK              0x01
#define TMP126_WRITE_MASK             0x00

#define TMP126_DEF_CONFIG_HIGH        0x00
#define TMP126_DEF_CONFIG_LOW     	  0xA6

#define TMP126_SENSITIVITY      	0.03125f;

static float TMP126_Temperature = 0.0f;

typedef enum _T_TMP126_REG_ADDR
{
    TMP126_REG_TEMPERATURE_RESULT = 0x00,
    TMP126_REG_SLEW_RESULT         = 0x01,
    TMP126_REG_ALERT_STATUS        = 0x02,
    TMP126_REG_CONFIGURATION       = 0x03,
    TMP126_REG_ALERT_ENABLE        = 0x04,
    TMP126_REG_TLOW_LIMIT          = 0x05,
    TMP126_REG_THIGH_LIMIT         = 0x06,
    TMP126_REG_HYSTERESIS          = 0x07,
    TMP126_REG_SLEW_LIMIT           = 0x08,
    TMP126_REG_UNIQUE_ID1           = 0x09,
    TMP126_REG_UNIQUE_ID2           = 0x0A,
    TMP126_REG_UNIQUE_ID3           = 0x0B,
    TMP126_REG_DEVICE_ID            = 0x0C
}
T_TMP126_REG_ADDR;

static void TMP126_SendSPI(SPI_HandleTypeDef* handle, uint8_t controlVal, uint8_t registerAddr, uint8_t messageHigh, uint8_t messageLow){

  uint8_t spiTxBuff1[2];
  uint8_t spiTxBuff2[2];

  // Bytes to write to EEPROM
  spiTxBuff1[0] = registerAddr;
  spiTxBuff1[1] = registerAddr;
  spiTxBuff2[0] = messageHigh;
  spiTxBuff2[1] = messageLow;

  LL_GPIO_ResetOutputPin(TMP126_CSPort, TMP126_CSPin);
  //osDelay(50);
  if (HAL_SPI_Transmit(handle, (uint8_t *)spiTxBuff1, 2, 100) != HAL_OK){

	  printf("SPI transmit1 error");

  }

  if (HAL_SPI_Transmit(handle, (uint8_t *)spiTxBuff2, 2, 100) != HAL_OK){

	  printf("SPI transmit2 error");

  }
  LL_GPIO_SetOutputPin(TMP126_CSPort, TMP126_CSPin);
  //osDelay(50);

}

static int TMP126_GetSPI(SPI_HandleTypeDef* handle, uint8_t controlVal, uint8_t registerAddr){

  uint8_t spiTxBuff[2];
  uint8_t spiRxBuff[2];
  uint16_t receivedHighByte = 0;
  uint16_t receivedLowByte = 0;
  uint16_t receivedData = 0;

  spiTxBuff[0] = controlVal;
  spiTxBuff[1] = registerAddr;
  LL_GPIO_ResetOutputPin(TMP126_CSPort, TMP126_CSPin);
  //osDelay(50);
  if (HAL_SPI_Transmit(handle, (uint8_t *)spiTxBuff, 2, 100) != HAL_OK){

	  printf("SPI transmitReceive error");

  }
  if (HAL_SPI_Receive(handle, (uint8_t *)spiRxBuff, 2, 100) != HAL_OK){

	  printf("SPI receive error");

  }
  LL_GPIO_SetOutputPin(TMP126_CSPort, TMP126_CSPin);
  //osDelay(50);

  receivedHighByte = spiRxBuff[0];
  receivedLowByte = spiRxBuff[1];

  receivedData = (receivedHighByte<<8)|receivedLowByte;

  return receivedData;
}

void TMP126_ConfigureSensor(void){

  TMP126_SendSPI(&TMP126_SPI_HANDLE, TMP126_WRITE_MASK, TMP126_REG_CONFIGURATION, TMP126_DEF_CONFIG_HIGH, TMP126_DEF_CONFIG_LOW);

}

int TMP126_GetID(void){

	return TMP126_GetSPI(&TMP126_SPI_HANDLE, TMP126_READ_MASK, TMP126_REG_UNIQUE_ID1);

}

float TMP126_MeasureTemp(void){

  int receivedValue = 0;

  receivedValue = TMP126_GetSPI(&TMP126_SPI_HANDLE, TMP126_READ_MASK, TMP126_REG_TEMPERATURE_RESULT);

  receivedValue = receivedValue >> 2;

  return (float)receivedValue*TMP126_SENSITIVITY;
}

float TMP126_GetTemp(void){

    return TMP126_Temperature;

}

int16_t TMP126_GetTempInt(void)
{
    return (int16_t)(TMP126_Temperature * 10);
}

void tmp126TaskEntry(void *argument)
{
    LOG_INFO("TMP126:: Task start");

    TMP126_ConfigureSensor();
    osDelay(pdMS_TO_TICKS(100));

    for (;;)
    {
        TMP126_Temperature = TMP126_MeasureTemp();
        osDelay(pdMS_TO_TICKS(200));
    }
}