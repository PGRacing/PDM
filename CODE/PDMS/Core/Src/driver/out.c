#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "logger.h"
#include "out.h"
#include "vmux.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "stdlib.h"
#include "cmsis_os2.h"
#include "ws2812b.h"
#include "pdm.h"

/// USER DEFINES
// Safety related defines
#define OUT_DIAG_READ_FREQ 200 // Hz frequency of ADC data provided via semaphore from ADC IRQ
#define OUT_DIAG_READ_PERIOD (1000 / OUT_DIAG_READ_FREQ)
#define OUT_SAFETY_OC_OFF 0xFFFF
#define OUT_DIAG_MS_TO_OC_TRIP(X) ((X) / OUT_DIAG_READ_PERIOD)
#define OUT_DIAG_BTS_OPEN_LOAD_TRESHOLD 20 // [mA] Under this value output channel current is considered open load
#define OUT_DIAG_BTS_DETECT_OPEN_LOAD TRUE // Is open load detection enabled on BTS channels?
#define OUT_DIAG_BTS_VBAT_HISTERESIS 1000 // [mV] Acceptable variation of voltage on output to Vbatt
#define OUT_DIAG_BTS_TRIP_THRESHOLD 40 // Arbitraty value that needs to be exceeded to trip SOC protection (removing super short local peaks)
// EMA
#define OUT_DIAG_EMA_CURRENT_ALPHA 0.2f // Alpha for exponential moving average of current, lower value means smoother readings but longer time to react to changes
#define OUT_DIAG_EMA_VOLTAGE_ALPHA 0.1f // Alpha for exponential moving average of voltage, lower value means smoother readings but longer time to react to changes

/// MACRO FUNCTIONS
#define OUT_ASSERT_IN_RANGE(id)      (ASSERT( (id) >= 0 && (id) < OUT_ID_MAX))
#define OUT_ASSERT_IN_RANGE_CFG(id)  (ASSERT( (id) >= 0 && (id) < ARRAY_COUNT(outsCfg)))
#define OUT_ASSERT_IN_RANGE_REG(id)  (ASSERT( (id) >= 0 && (id) < ARRAY_COUNT(outsReg)))

#define OUT_STATUS_GET_HIGHER_PRIORITY(prioa, priob) ((prioa > priob) ?  prioa : priob)

/// FUNCTION PROTOTYPES
static T_OUT_CFG* OUT_GetCfgPtr( T_OUT_ID id );
static T_OUT_REG* OUT_GetRegPtr( T_OUT_ID id );
static inline void OUT_DIAG_ArmSocProtection(T_OUT_ID id);

// Get current time in ms
#define OUT_GET_TIME_MS (pdTICKS_TO_MS( xTaskGetTickCount() ))

// do + while(0) here for correct macro evaluation
#define OUT_RETRY_CALLBACK_BODY(id)   do{ OUT_SetState((id), OUT_STATE_ON); \
                                          OUT_GetRegPtr((id))->safety.inRetrySequence = FALSE; }while(0)

#pragma region RETRY_CALLBACKS

void OUT_CH1_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_1);
}
void OUT_CH2_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_2);
}
void OUT_CH3_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_3);
}
void OUT_CH4_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_4);
}
void OUT_CH5_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_5);
}
void OUT_CH6_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_6);
}
void OUT_CH7_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_7);
}
void OUT_CH8_RetryCallback(void)
{
  OUT_RETRY_CALLBACK_BODY(OUT_ID_8);
}

#pragma endregion

#pragma region OUTPUT_CONFIG

#if BOARD_VER == PDMS_V4_2
/// @brief Main output channels config [1..16]
T_OUT_CFG outsCfg[OUT_ID_MAX] =
{
        [OUT_ID_1] = {
            .id = OUT_ID_1,
            .name = "SERVO",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_LATCH 
              },
              //.afterErrorCfg = OUT_ERR_BEH_RETRY,
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 10000, // mA
              // .socTripThreshold = 400, // 4 x ms
              .errRetryThreshold = 3, // 
           // .retryTimerInterval = 1000, 
              .retryTimerInterval = 3000,
              .retryCallback = &OUT_CH1_RetryCallback,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 2500,
                .allowInrush = TRUE,
                .inrushWindowFromStart = UINT32_MAX,
                .inrushTimeThreshold = 500,
                .inrushThreshold = 4000
              }
            }
        },
        [OUT_ID_2] = {
            .id = OUT_ID_2,
            .name = "CAN L",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              // .useSoc = TRUE,
              // .socThreshold = 2000, // 2000mA Threshold
              // .socTripThreshold = 0,
              .retryCallback = &OUT_CH2_RetryCallback,
            }
        },
        [OUT_ID_3] = {
            .id = OUT_ID_3,
            .name = "GCU",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 8000, // mA
              // .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .retryCallback = &OUT_CH3_RetryCallback,
            }
        },
        [OUT_ID_4] = {
            .id = OUT_ID_4,
            .name = "FAN1",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              {
                .behavior = OUT_ERR_BEH_LATCH
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 20000, // mA
              // .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .retryCallback = &OUT_CH4_RetryCallback,
            }
        },
        [OUT_ID_5] = {
            .id = OUT_ID_5,
            .name = "MAINS",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_LATCH
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 12000, // mA
              // .socTripThreshold = 4000, // 4 x ms
              .errRetryThreshold = 6, // 
              .retryTimerInterval = 1000,
              .retryCallback = &OUT_CH5_RetryCallback,
            }
        },
        [OUT_ID_6] = {
            .id = OUT_ID_6,
            .name = "PUMP",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_LATCH
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 10000, // mA
              // .socTripThreshold = 2600, // 4 x ms
              .errRetryThreshold = 6, // 
              .retryTimerInterval = 1000,
              .retryCallback = &OUT_CH6_RetryCallback,
            }
        },
        [OUT_ID_7] = {
            .id = OUT_ID_7,
            .name = "FAN2",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              {
                .behavior = OUT_ERR_BEH_LATCH
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 20000, // mA
              // .socTripThreshold = 1600, // 4 x ms
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .retryCallback = &OUT_CH7_RetryCallback,
            }
        },
        [OUT_ID_8] = {
            .id = OUT_ID_8,
            .name = "ECU",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              // .useSoc = TRUE,
              // .socThreshold = 5000, // mA
              // .socTripThreshold = 1200, // 4 x ms
              .errRetryThreshold = 6, // 
              .retryTimerInterval = 2000,
              .retryCallback = &OUT_CH8_RetryCallback,
            }
        },
        [OUT_ID_9] = {
            .id = OUT_ID_9,
            .name = "TELE B",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_10] = {
            .id = OUT_ID_10,
            .name = "E.LOG",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_11] = {
            .id = OUT_ID_11,
            .name = "TELE F",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_12] = {
            .id = OUT_ID_12,
            .name = "DASH",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_13] = {
            .id = OUT_ID_13,
            .name = "COMM",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_14] = {
            .id = OUT_ID_14,
            .name = "C.LOG",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_15] = {
            .id = OUT_ID_15,
            .name = "ADD 1",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_16] = {
            .id = OUT_ID_16,
            .name = "ADD 2",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },        

};
#elif BOARD_VER == PDMS_V4_3

/// @brief Main output channels config [1..16]
T_OUT_CFG outsCfg[OUT_ID_MAX] =
{
        [OUT_ID_1] = {
            .id = OUT_ID_1,
            .name = "-",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            // .safety = 
            // {
            //   .afterErrorCfg = 
            //   { 
            //     .behavior = OUT_ERR_BEH_RETRY 
            //   },
            //   .actOnSafety = FALSE,
            //   .errRetryThreshold = 3,
            //   .retryTimerInterval = 1000,
            //   .socCfg =
            //   {
            //     .useSoc = TRUE,
            //     .nominalThreshold = 10000, // mA
            //     .allowInrush = TRUE,
            //     .inrushWindowFromStart = 2000,
            //     .inrushTimeThreshold = 2000, // ms
            //     .inrushThreshold = 20000 // mA
            //   }
            // }
        },
        [OUT_ID_2] = {
            .id = OUT_ID_2,
            .name = "-",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_LATCH 
              },
              .actOnSafety = FALSE,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 2000, // mA
                .allowInrush = FALSE,
              }              
            }
        },
        [OUT_ID_3] = {
            .id = OUT_ID_3,
            .name = "ECU",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 10000, // (max.) 10A DBW + 2A ECU + 2A LAMBDA
                .allowInrush = FALSE,
              }
            }
        },
        [OUT_ID_4] = {
            .id = OUT_ID_4,
            .name = "FAN1",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              {
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 10000, // mA
                .allowInrush = TRUE,
                .inrushWindowFromStart = UINT32_MAX,
                .inrushTimeThreshold = 2000, // ms
                .inrushThreshold = 30000 // mA
              }
            }
        },
        [OUT_ID_5] = {
            .id = OUT_ID_5,
            .name = "COILS",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 6, 
              .retryTimerInterval = 1000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 15000, // mA
                .allowInrush = FALSE,
              }
            }
        },
        [OUT_ID_6] = {
            .id = OUT_ID_6,
            .name = "PUMP",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 6,  
              .retryTimerInterval = 1000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 10000, // mA
                .allowInrush = TRUE,
                .inrushWindowFromStart = 5000, //ms
                .inrushTimeThreshold = 1000, // ms
                .inrushThreshold = 25000 // mA
              }
            }
        },
        [OUT_ID_7] = {
            .id = OUT_ID_7,
            .name = "INJECTOR",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = 
            {
              .afterErrorCfg = 
              {
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 3, // 
              .retryTimerInterval = 1000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 10000, // mA
                .allowInrush = TRUE,
                .inrushWindowFromStart = UINT32_MAX,
                .inrushTimeThreshold = 2000, // ms
                .inrushThreshold = 30000 // mA
              }
            }
        },
        [OUT_ID_8] = {
            .id = OUT_ID_8,
            .name = "SERVO",
            .type = OUT_TYPE_BTS500,
            .mode = OUT_MODE_UNUSED,
            .safety = {
              .afterErrorCfg = 
              { 
                .behavior = OUT_ERR_BEH_RETRY
              },
              .actOnSafety = FALSE,
              .errRetryThreshold = 6, 
              .retryTimerInterval = 2000,
              .socCfg =
              {
                .useSoc = TRUE,
                .nominalThreshold = 15000,
                .allowInrush = FALSE,
              }
            }
        },
        [OUT_ID_9] = {
            .id = OUT_ID_9,
            .name = "ADD 1",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_10] = {
            .id = OUT_ID_10,
            .name = "ADD 2",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_11] = {
            .id = OUT_ID_11,
            .name = "ADD 3",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_12] = {
            .id = OUT_ID_12,
            .name = "ADD 4",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_1,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },
        [OUT_ID_13] = {
            .id = OUT_ID_13,
            .name = "ADD 5",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_1,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_14] = {
            .id = OUT_ID_14,
            .name = "ADD 6",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_2,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_15] = {
            .id = OUT_ID_15,
            .name = "ADD 7",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_3,
            .mode = OUT_MODE_UNUSED,
        },        
        [OUT_ID_16] = {
            .id = OUT_ID_16,
            .name = "EGT2CAN",
            .type = OUT_TYPE_SPOC2,
            .spocId = SPOC2_ID_2,
            .spocChId = SPOC2_CH_ID_4,
            .mode = OUT_MODE_UNUSED,
        },        

};
#endif

#pragma endregion

#pragma region OUTPUT_REGISTER_INIT

/// @brief Main output channel status register
T_OUT_REG outsReg[OUT_ID_MAX] =
{
  [OUT_ID_1] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_2] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_3] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_4] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_5] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_6] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_7] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_8] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_9] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_10] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_11] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_12] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_13] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_14] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_15] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  },
  [OUT_ID_16] = 
  {
    .state = OUT_STATE_OFF,
    .status = OUT_STATUS_OK,
    .safety = 
    {
      .errRetryCounter = 0,
      .retryTimerCounter = 0,
      .inRetrySequence = FALSE,
    },
    .currentMA = 0,
    .voltageMV = 0
  }
};


void  (*outsRetryCallbacks[OUT_ID_MAX])(void) = 
{
  [OUT_ID_1] = &OUT_CH1_RetryCallback,
  [OUT_ID_2] = &OUT_CH2_RetryCallback,
  [OUT_ID_3] = &OUT_CH3_RetryCallback,
  [OUT_ID_4] = &OUT_CH4_RetryCallback,
  [OUT_ID_5] = &OUT_CH5_RetryCallback,
  [OUT_ID_6] = &OUT_CH6_RetryCallback,
  [OUT_ID_7] = &OUT_CH7_RetryCallback,
  [OUT_ID_8] = &OUT_CH8_RetryCallback,
  [OUT_ID_9] = NULL,
  [OUT_ID_10] = NULL,
  [OUT_ID_11] = NULL,
  [OUT_ID_12] = NULL,
  [OUT_ID_13] = NULL,
  [OUT_ID_14] = NULL,
  [OUT_ID_15] = NULL,
  [OUT_ID_16] = NULL
};

#pragma endregion

// Acquire config struct
#define OUT_GETCFGPTR(id) (&(outsCfg[(id)]))

/// @brief Acquire configuration struct with range assert
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Pointer to configuration struct
static T_OUT_CFG* OUT_GetCfgPtr( T_OUT_ID id )
{
  OUT_ASSERT_IN_RANGE_CFG(id);
  return &(outsCfg[id]);
}

/// Acquire status register of selected channel
#define OUT_GETREGPTR(id) (&(outsReg[(id)]))

/// @brief Acquire status register of selected channel with range assert
/// @param id Output channel id [1..16] T_OUT_ID
/// @return Pointer to status register
static T_OUT_REG* OUT_GetRegPtr( T_OUT_ID id )
{
  OUT_ASSERT_IN_RANGE_REG(id);
  return &(outsReg[id]);
}

void OUT_ChangeMode(T_OUT_ID id, T_OUT_MODE targetMode)
{
  T_OUT_CFG* cfg = OUT_GetCfgPtr(id);
  ASSERT( cfg );

  switch (targetMode)
  {
  case OUT_MODE_UNUSED:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
       if (cfg->mode == OUT_MODE_PWM)
      {
        BSP_OUT_DeInitPWM(id);
      }
      BSP_OUT_SetMode(id, targetMode);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      // TODO [LOW] Add handler
      //SPOC2_SetMode(id, targetMode);
    }
    cfg->mode = targetMode;
    break;
  }
  case OUT_MODE_STD:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      if (cfg->mode == OUT_MODE_PWM)
      {
        BSP_OUT_DeInitPWM(id);
      }
      BSP_OUT_SetMode(id, targetMode);
    }
    else if(cfg->type == OUT_TYPE_SPOC2)
    {
      // TODO [LOW] Add handler
      // SPOC2_SetMode(id, targetMode);
    }
    OUT_SetState(cfg->id, OUT_STATE_OFF);
    cfg->mode = targetMode;
    break;
  }
  case OUT_MODE_PWM:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      BSP_OUT_SetMode(id, targetMode);
      // Set PWM duty to 0%
      OUT_SetDutyPWM(id, 0);
      cfg->mode = targetMode;
    }
    break;
  }
  case OUT_MODE_BATCH:
  {
    if(cfg->type == OUT_TYPE_BTS500)
    {
      LOG_WARN("Batching two inputs!");

      /* Batch shouldn't be out of range */
      ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));
      T_OUT_CFG *batchCfg = &(outsCfg[cfg->batch]);

      // LL MODE STD
      BSP_OUT_SetMode(id, OUT_MODE_STD);
      BSP_OUT_SetMode(cfg->batch, OUT_MODE_STD);
      OUT_SetState(cfg->id, OUT_STATE_OFF);
      OUT_SetState(batchCfg->id, OUT_STATE_OFF);
      batchCfg->mode = OUT_MODE_BATCH;
      /* From now on actions on batch outputs are performed simultaniously */
      cfg->mode = targetMode;
    }
    break;
  }

  default:
    break;
  }

  return;
}

void OUT_Reconfigure(T_OUT_ID id)
{
  T_OUT_CFG* cfg = OUT_GetCfgPtr(id);
  ASSERT( cfg );

  OUT_ChangeMode(id, cfg->mode);
}

bool OUT_SetDutyPWM(T_OUT_ID id, uint8_t duty)
{
  bool res = TRUE;

  OUT_ASSERT_IN_RANGE(id);

  // Acquire output channel status reg
  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  if(OUT_GetMode(id) == OUT_MODE_PWM && reg->state == OUT_STATE_ON)
  {
    reg->prevPwmDuty = reg->pwmDuty;
    reg->pwmDuty = duty;
  
    // Apply new PWM duty
    BSP_OUT_SetDutyPWM(id, duty);
  }
  else
  {
    // Set duty for next turn-on
    reg->prevPwmDuty = duty;
  }

  return res; // Always TRUE
}

static uint8_t OUT_InterpolatePWM(const uint16_t xAxis[OUT_PWM_MAP_RESOLUTION], const uint8_t yAxis[OUT_PWM_MAP_RESOLUTION], uint16_t x)
{
  // Clamp below range
  if (x <= xAxis[0])
      return yAxis[0];

  // Clamp above range
  if (x >= xAxis[OUT_PWM_MAP_RESOLUTION - 1])
      return yAxis[OUT_PWM_MAP_RESOLUTION - 1];

  // Find segment
  for (int i = 0; i < OUT_PWM_MAP_RESOLUTION - 1; i++)
  {
      if (x <= xAxis[i + 1])
      {
          float t = (x - xAxis[i]) /
                    (xAxis[i + 1] - xAxis[i]);

          return yAxis[i] +
                t * (yAxis[i + 1] - yAxis[i]);
      }
  }

  return yAxis[OUT_PWM_MAP_RESOLUTION - 1];
}

bool OUT_SetState(T_OUT_ID id, T_OUT_STATE reqState)
{
  bool res = TRUE;

  OUT_ASSERT_IN_RANGE(id);

  // Acquire output channel config
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  // Acquire output channel status reg
  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  // If channel is latched it cannot be used
  if(reg->state == OUT_STATE_ERR_LATCH)
  {
    return FALSE;
  }

  // If channel in error handler it cannot be turned on manually
  if(reg->safety.inRetrySequence == TRUE 
    && reqState == OUT_STATE_ON)
  {
    return FALSE;
  }

  // New state will be error latch (perform channel shutdown and block)
  if(reqState == OUT_STATE_ERR_LATCH)
  {
    OUT_SetState(id, OUT_STATE_OFF);
    reg->state = OUT_STATE_ERR_LATCH;
    return FALSE;
  }

  // If state changed - apply new state if possible
  if(reqState != reg->state)
  {
    switch (cfg->mode)
    {
      case OUT_MODE_UNUSED:
      {
        LOG_WARN("Unable to set state on unused output config");
        res = FALSE;
      }
      break;

      case OUT_MODE_STD:
      {
        if(cfg->type == OUT_TYPE_BTS500)
        {
          // ARM software over current protection
          if(OUT_STATE_ON == reqState)
          {
            OUT_DIAG_ArmSocProtection(id);
          }
          BSP_OUT_SetStdState(id, reqState);
        }
        else if(cfg->type == OUT_TYPE_SPOC2)
        {
          SPOC2_SetStdState(cfg->spocId, cfg->spocChId, reqState);
        }
        reg->state = reqState;
        res = TRUE;
      }
      break;

      case OUT_MODE_PWM:
      { 
          if(OUT_STATE_OFF == reqState)
          {
            // Disable channel with storing previous value
            OUT_SetDutyPWM(id, 0);
          }
          else if(OUT_STATE_ON == reqState)
          {
            // ARM software over current protection
            OUT_DIAG_ArmSocProtection(id);
            // Restore previous PWM duty
            reg->pwmDuty = reg->prevPwmDuty;
            BSP_OUT_SetDutyPWM(id, reg->prevPwmDuty);
          }
          reg->state = reqState;
          res = TRUE;
      }
      break;

      case OUT_MODE_BATCH:
      {
        if(cfg->type == OUT_TYPE_BTS500)
        {
          ASSERT(cfg->batch < ARRAY_COUNT(outsCfg));
          T_OUT_REG* batchReg = OUT_GETREGPTR(cfg->batch);

          if(OUT_STATE_ON == reqState)
          {
            OUT_DIAG_ArmSocProtection(id);
            OUT_DIAG_ArmSocProtection(cfg->batch);
          }
          // Use this function to change GPIO register simultaniously for both switches
          BSP_OUT_SetBatchState(id, cfg->batch, reqState);

          reg->state = reqState;
          batchReg->state = reqState;
          res = TRUE;
        }
        else
        {
          res = FALSE;
        }
      }
      break;

      default:
        break;
    }
  }
  else if(cfg->mode == OUT_MODE_PWM && reqState == OUT_STATE_ON)
  {
    if(cfg->pwmCfg.dutyInput != INPUT_ID_UNASSIGNED_VALUE)
    {
      // Set PWM value based on assigned input
      uint8_t newDuty = OUT_InterpolatePWM(cfg->pwmCfg.inputAxis, cfg->pwmCfg.dutyAxis, IN_GetValueAnalog(cfg->pwmCfg.dutyInput));
      OUT_SetDutyPWM(id, newDuty);
    }
    else
    {
      OUT_SetDutyPWM(id, cfg->pwmCfg.baseDuty);
    }
  }

  return res;
}

bool OUT_Batch(T_OUT_ID id, T_OUT_ID batchId)
{
  /// TODO [LOW] Add handling of SPOC2 internal batch function
  T_OUT_CFG* cfg = OUT_GetCfgPtr(id);
  T_OUT_CFG* batchCfg = OUT_GetCfgPtr(batchId);
  ASSERT(cfg);
  ASSERT(batchCfg);

  if(cfg->type != OUT_TYPE_BTS500 || batchCfg->type != OUT_TYPE_BTS500)
  {
    LOG_ERR("Batch working only on BTS500");
    return FALSE;
  }

  if (batchId >= ARRAY_COUNT(outsCfg))
  {
    LOG_ERR("Setting batch out of range!");
    return FALSE;
  }

  if(!BSP_OUT_IsBatchPossible(id, batchId))
  {
    LOG_ERR("Batch with diffrent port");
    return FALSE;
  }

  cfg->batch = batchId;
  batchCfg->batch = id;

  OUT_ChangeMode(id, OUT_MODE_BATCH);
  return TRUE;
}

bool OUT_ToggleState(T_OUT_ID id)
{
  T_OUT_REG* reg = OUT_GetRegPtr(id);
  ASSERT(reg);
  return OUT_SetState( id, !reg->state);
}

#pragma region DIAG_SECTION

/// @brief If TRY_RETRY is used this function will take care of retry routine
/// @param id Output channel id [1..16] T_OUT_ID
static void OUT_DIAG_DispatchRetry(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);

  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  if (reg->safety.errRetryCounter >= cfg->safety.errRetryThreshold)
  {
    // Disable timer and latch output channel
    OUT_SetState(id, OUT_STATE_ERR_LATCH);
  }
  else if (FALSE == reg->safety.inRetrySequence)
  {
    reg->safety.errRetryCounter++;
  
    // Reset timer counter for retry execution routine
    reg->safety.retryTimerCounter = 0;

    // Set-off retry sequence counter 
    reg->safety.inRetrySequence = TRUE;
  }
  return;
}

/// @brief Callback executed if switch turned to error, execute proper error handling routine
/// @param id Output channel id [1..16] T_OUT_ID
static void OUT_DIAG_OnErrorFallback(T_OUT_ID id)
{

  OUT_ASSERT_IN_RANGE(id);
  const T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  switch (cfg->safety.afterErrorCfg.behavior)
  {
  case OUT_ERR_BEH_NO:
    break;

  case OUT_ERR_BEH_TIME_LATCH: // Not implemented
  case OUT_ERR_BEH_LATCH:
    // Latch channel in off state till device reset
    OUT_SetState(id, OUT_STATE_ERR_LATCH);
    break; 

  case OUT_ERR_BEH_RETRY:
    OUT_SetState(id, OUT_STATE_OFF);
    OUT_DIAG_DispatchRetry(id);
    break;

  default:
    break;
  }
}

/// @brief Arm software over current protection system
/// @param id 
/// @note Should be performed when changing output state to ON 
static inline void OUT_DIAG_ArmSocProtection(T_OUT_ID id)
{
  // Set status as pre inrush
  outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_PRE_INRUSH;
  
  // Set timer counters to 0
  outsReg[id].safety.socReg.inrushTripCounter = 0;
  outsReg[id].safety.socReg.timeInPreInrush = 0;
  outsReg[id].safety.socReg.thresholdExceedCounter = 0;

  if(outsCfg[id].safety.socCfg.allowInrush == true)
  {
    outsReg[id].safety.socReg.currentThreshold = outsCfg[id].safety.socCfg.inrushThreshold;
  }
  else
  {
    outsReg[id].safety.socReg.currentThreshold = outsCfg[id].safety.socCfg.nominalThreshold;
    outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_NORMAL_OPERATION;
  }
  
}

/// @brief Software over current protection processing executed in fast loop
/// @param id Output channel id [1..16] T_OUT_ID
/// @param state Output channel state [ON/OFF]
/// @param voltageMV Output voltage [mV]
/// @param currentMA Output current [mA]
/// @return New status of the channel
/// @note Mode of operation:
///
/// 1. Channel is set ON - ARM software over current protection
/// 2. Check if inrush function is enabled (enableInrush)
///     2.1. Set Ith = Iinr
///     2.2. During Tallinr wait till I >= In - wait for first current peak during allowed time (Tallinr)
///     2.3. Start timer counting down from Tinr to 0
///     2.4  During this time check if I >= Iinr, if yes disable channel ASAP if no do nothing
/// 3. Set Ith = In
/// 4. If I >= Ith disable channel asap, otherwise allow normal work
static inline T_OUT_STATUS OUT_DIAG_SocProtection(T_OUT_ID id, T_OUT_STATE state,  uint32_t voltageMV, uint32_t currentMA)
{

  T_OUT_STATUS status = OUT_STATUS_OK;
  // 1. Channel set ON
  if (TRUE == outsCfg[id].safety.socCfg.useSoc && OUT_STATE_ON == state)
  {
    // 1.1 Check if current exceeds threshold
    if (currentMA >= outsReg[id].safety.socReg.currentThreshold)
    {
      if(outsReg[id].safety.socReg.thresholdExceedCounter > OUT_DIAG_BTS_TRIP_THRESHOLD)
      {
        outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_TRIGGERED;
        status = OUT_STATUS_SOC_FAULT;
        // This brakes the rules of coding style, but is used to act as fast as possible
        return status;
      }
      else
      {
        outsReg[id].safety.socReg.thresholdExceedCounter += 4;
      }
    }
    // 1.2 If in inrush mode, check for first current peak
    else if (outsCfg[id].safety.socCfg.allowInrush == true)
    {
      if (OUT_SAFETY_SOC_PRE_INRUSH == outsReg[id].safety.socReg.status)
      {
        // 1.2.1 Await for first peak
        if (currentMA >= outsCfg[id].safety.socCfg.nominalThreshold)
        {
          // Set off timer
          outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_INRUSH_WINDOW;
          outsReg[id].safety.socReg.inrushTripCounter++;
        }
        else
        {
          // 1.2.2 If no peak current, increment time counter
          outsReg[id].safety.socReg.timeInPreInrush++;
          
          // 1.2.3 If counter passes threshold, go into normal operation with lower threshold
          if (outsReg[id].safety.socReg.timeInPreInrush >= (outsCfg[id].safety.socCfg.inrushWindowFromStart/OUT_DIAG_READ_PERIOD))
          {
            outsReg[id].safety.socReg.currentThreshold = outsCfg[id].safety.socCfg.nominalThreshold;
            outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_NORMAL_OPERATION;
          }
        }
      }
      else if (OUT_SAFETY_SOC_INRUSH_WINDOW == outsReg[id].safety.socReg.status)
      {
        // 1.3 If during inrush window, increment timer counter
        outsReg[id].safety.socReg.inrushTripCounter++;

        // 1.3.1 If counter passes threshold, go into normal operation with lower threshold
        if (outsReg[id].safety.socReg.inrushTripCounter >= (outsCfg[id].safety.socCfg.inrushTimeThreshold/OUT_DIAG_READ_PERIOD))
        {
          outsReg[id].safety.socReg.currentThreshold = outsCfg[id].safety.socCfg.nominalThreshold;
          outsReg[id].safety.socReg.status = OUT_SAFETY_SOC_NORMAL_OPERATION;
        }
      }
    }

    if (currentMA < outsReg[id].safety.socReg.currentThreshold 
      && outsReg[id].safety.socReg.thresholdExceedCounter > 0)
    {
      outsReg[id].safety.socReg.thresholdExceedCounter -= 1;
    }
  }
  return status;
}

static inline T_OUT_STATUS OUT_DIAG_I2tProtection(T_OUT_ID id, T_OUT_STATE state,  uint32_t voltageMV, uint32_t currentMA)
{
  /*
  1. If current over target add to sum current * current
  2. If current under threshold sub sum current * current
  3. Determine threshold as I*2 * t
  */

  T_OUT_STATUS status = OUT_STATUS_OK;

  // We need to scale down the resolution to fit in int32  
  int32_t iPart = (currentMA / 10) * (currentMA / 10) - (outsCfg[id].safety.i2tCfg.nominalCurrentSq);
  // TODO Convert to float and ceil(iPart);
  
  if(outsReg[id].safety.i2tReg.i2tSum > 0)
  {
    outsReg[id].safety.i2tReg.i2tSum += iPart;
  }
  
  if(outsReg[id].safety.i2tReg.i2tSum >= outsCfg[id].safety.i2tCfg.i2tThreshold)
  {
    status = OUT_STATUS_I2T_FAULT;
  }
  
 return status;
}

/// @brief Hardware assessment of BTS500 output channel
/// @param id Output channel id [1..16] T_OUT_ID
/// @param state Output channel state [ON/OFF]
/// @param voltageMV Output voltage [mV]
/// @param currentMA Output current [mA]
/// @param inFault Check if channel is in fault state
/// @return New status of the channel
/// @note This funciton is used with BTS500 type of output channels
static inline T_OUT_STATUS OUT_DIAG_BtsHardware(T_OUT_ID id, T_OUT_STATE state, uint32_t voltageMV, uint32_t currentMA, uint32_t inFault)
{
  T_OUT_STATUS newStatus = OUT_STATUS_OK;

  uint32_t diffChVbat = abs((int32_t)(VMUX_GetBattValue() - voltageMV));
  bool noLoad  = currentMA < OUT_DIAG_BTS_OPEN_LOAD_TRESHOLD;

  // Check conditions in on state
  if(OUT_STATE_ON == state) 
  {
    // Check if channel output voltage is around battery voltage
    if(diffChVbat < OUT_DIAG_BTS_VBAT_HISTERESIS)
    {
      // Check if open-load detection is enabled and no load condition is met
      if(OUT_DIAG_BTS_DETECT_OPEN_LOAD && (TRUE == noLoad))
      {
        newStatus = OUT_STATUS_OPEN_LOAD;
      }
      else
      {
        newStatus = OUT_STATUS_OK;
      }
    }
    // Channel was disabled or pulled down to ground
    else
    {
      // Check fault condition
      if(TRUE == inFault)
      {
        newStatus = OUT_STATUS_HARD_FAULT; 
      }
      // If output is not in fault and disabled something is wrong
      // MCU is not able to control output channel
      else
      {
        // TODO Some control error occurs due to not sufficent data from VMUX currently won't be used
        //newStatus = OUT_STATUS_CONTROL_FAIL;
        newStatus = OUT_STATUS_OK;
      }
    }
  }
  // Diagnosis in off state
  else if(OUT_STATE_OFF == state)
  {
    // Check if channnel is shorted to ground
    if(diffChVbat < OUT_DIAG_BTS_VBAT_HISTERESIS)
    {
      // During switching inductive load it takes some time to discharge
      //newStatus = OUT_STATUS_SHORT_TO_VSS;
    }
    else
    {
      newStatus = OUT_STATUS_OK;
    }
  }

  return newStatus;
}

static void OUT_DIAG_SingleBtsNew(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  T_OUT_STATUS newStatus = OUT_STATUS_OK;

  // Get into critical section for PHY parameters readout
  vPortEnterCritical();
  
  // Calculate current from ADC raw data
  if(OUT_STATE_ON == reg->state)
  {
    // If channel is ON calculate current
    reg->currentMA = BSP_OUT_CalcCurrent(id);
    reg->emaCurrentMA = (reg->currentMA * OUT_DIAG_EMA_CURRENT_ALPHA) + (reg->emaCurrentMA * (1 - OUT_DIAG_EMA_CURRENT_ALPHA));
  }
  else
  {
    // If channel is OFF override current to 0
    reg->currentMA = 0;
    reg->emaCurrentMA = (1 - OUT_DIAG_EMA_CURRENT_ALPHA) * reg->emaCurrentMA;
  }
  bool inFault = BSP_OUT_IsCurrentFault(id);
  
  // Get channel voltage from voltage multiplexer ADC data (already calculated)
  reg->voltageMV = VMUX_GetValue(id);
  reg->emaVoltageMV = (reg->voltageMV * OUT_DIAG_EMA_VOLTAGE_ALPHA) + (reg->emaVoltageMV * (1 - OUT_DIAG_EMA_VOLTAGE_ALPHA));

  // Check for hardware issues and state changes
  T_OUT_STATUS hwStatus = OUT_DIAG_BtsHardware(id, reg->state, reg->voltageMV, reg->currentMA, inFault);

  // Check for software overcurrent
  T_OUT_STATUS swStatus = OUT_DIAG_SocProtection(id, reg->state, reg->voltageMV, reg->currentMA);

  // Check safety line
  T_OUT_STATUS safetyStatus = OUT_STATUS_OK;
  if(OUT_STATE_ON == reg->state && reg->voltageMV < OUT_DIAG_BTS_VBAT_HISTERESIS && FALSE == PDM_GetSafetyState())
  {
    safetyStatus = OUT_STATUS_SAFETY_OPEN;
  }

  newStatus = OUT_STATUS_GET_HIGHER_PRIORITY(safetyStatus, swStatus);
  newStatus = OUT_STATUS_GET_HIGHER_PRIORITY(hwStatus, newStatus);

  // If any error act as selected in configuration
  if(newStatus >= OUT_STATUS_PRIORITY_DIV)
  {
    OUT_DIAG_OnErrorFallback(id);
  }

  // Perform retry procedure
  if( TRUE == reg->safety.inRetrySequence)
  {
    reg->safety.retryTimerCounter++;

    // If timer counter exceeded threshold call retry callback
    if(reg->safety.retryTimerCounter >= (cfg->safety.retryTimerInterval/OUT_DIAG_READ_PERIOD))
    {
      // Check if safety function callback does exist
      ASSERT(outsRetryCallbacks[id]);

      outsRetryCallbacks[id]();
    }
  }

  // Set new channel status
  reg->status = newStatus;

  // Exit critical section
  vPortExitCritical();
}

// // TOD_OLD Change it to modular (this should be protection entry)
// /// @brief Perform all needed processing for output channel of BTS type
// /// @param id Output channel id [1..8] T_OUT_ID
// static void OUT_DIAG_SingleBts(T_OUT_ID id)
// {
//   // TOD_OLD Perform major rework based on comments and notion document
//   OUT_ASSERT_IN_RANGE(id);
//   // TOD_OLD First fix state machine
  
//   T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
//   ASSERT(cfg);

//   T_OUT_REG* reg = OUT_GETREGPTR(id);
//   ASSERT(reg);

//   T_OUT_STATUS newStatus = OUT_STATUS_OK;

//   // BTS500 ONLY
//   if( outsCfg[id].type == OUT_TYPE_BTS500)
//   {

//     /// SOFT OC BLOCK START
//     // TOD_OLD Access of current should be propably in critical section or some sync should be used, write to memory by DMA, when reading?
//     // Not a real concern in case of in ISR execution on ADC
//     // What if ADC readout is wrong or ADC is malfunctioning? Assure ADC safety
//     // We do soft overcurrent block but we don't now current switch state (maybe hard oc), should be changed
//     reg->currentMA = BSP_OUT_CalcCurrent(id);  

//     if(TRUE == cfg->safety.useSoc)
//     {
//       if(reg->currentMA > cfg->safety.socThreshold)
//       {
//         if(reg->safety.ocTripCounter >= cfg->safety.socTripThreshold)
//         {
//           newStatus = OUT_STATUS_SOC_FAULT;
//           // TOD_OLD Make imidiate action on oc detection!!!
//           reg->safety.ocTripCounter = 0;
//         }else
//         {
//           if( OUT_STATE_ON == reg->state)
//           {
//             // TODO Shouldn't be increased by const implement some fusing current
//             reg->safety.ocTripCounter += 4;
//           }
//         }
//       }
//       else
//       {
//         if(reg->safety.ocTripCounter > 0)
//         {
//           reg->safety.ocTripCounter--;
//         }
//       }
//     }
//     /// SOFT OC BLOCK END

//     // TODO Implement I2t and heat protection block
    
//     /// STATE DETECTION BLOCK START
//     // TODO How is voltage synced with current adc readout???
//     // What in case of ADC error or VMUX malfunction
//     // Add better filtering
//     reg->voltageMV = VMUX_GetValue(id);
//     //uint32_t fault_level = BSP_OUT_GetDkilis(id) * 4;
//     uint32_t batteryVoltage = VMUX_GetBattValue();
//     // TOD_OLD This fault level should be changed 
//     // TOD_OLD If new zenner diodes installed it should be variable due to used dkilis and resistor
//     // TOD_OLD Change for better state detection
//     uint32_t faultLevel = 12000;
//     // TOD_OLD Remove not needed call just to access const memory location (performance issue)
//     uint32_t dkilis = BSP_OUT_GetDkilis(id);
//     // TOD_OLD Changed this value experimentally 
//     uint32_t voltageHis = 1000; // 1000 mV for now

//     T_OUT_STATUS hwStatus = OUT_STATUS_OK;
//     if(OUT_STATE_OFF == reg->state)
//     {
//         if(reg->currentMA >= faultLevel)
//         { 
//           if(abs((int32_t)(batteryVoltage - reg->voltageMV) < voltageHis))
//           {
//             hwStatus = OUT_STATUS_SHORT_TO_VSS;
//           }
//           else
//           {
//             hwStatus = OUT_STATUS_OK;
//           }
//         }
//     }
//     else
//     {
//       // TOD_OLD Fix magic values
//         if(reg->currentMA >= faultLevel)
//         {
//           // TOD_OLD WARN
//           // No latch on HW
//           //hwStatus = OUT_STATUS_HARD_OC_OR_OT;
//         }
//         // This is shit some in ampers some in mili amps
//         else if(((reg->currentMA <= 0.0000143 * (float)dkilis) && (reg->currentMA > 0.000001 * (float)dkilis)) 
//         || (reg->currentMA <= 0.000001 * (float)dkilis))
//         {
//           // TOD_OLD Fix status frequent change on lower current
//           hwStatus = OUT_STATUS_OPEN_LOAD;
//         }
//         // 12000 = 12V
//         else if((reg->voltageMV <= batteryVoltage) && (reg->currentMA > 0.0000143 * (float)dkilis))
//         {
//           hwStatus = OUT_STATUS_OK;
//         }
//     }
//     /// STATE DETECTION BLOCK END

//     vPortEnterCritical();
//     // TOD_OLD Change this to proper state machine
//     // TODO Value such over current or over temperature (even software one) should be hold till restart
//     T_OUT_STATUS prevStatus = reg->status;
//     if(newStatus == OUT_STATUS_SOC_FAULT && hwStatus == OUT_STATUS_HARD_FAULT)
//     {
//     reg->status =  OUT_STATUS_HARD_FAULT;
//     }else if(newStatus == OUT_STATUS_SOC_FAULT)
//     {
//       reg->status = newStatus;
//     }else
//     {
//       reg->status = hwStatus;
//     }

//     if( reg->status != prevStatus )
//     {
//       if(reg->status == OUT_STATUS_HARD_FAULT ||
//           reg->status == OUT_STATUS_SOC_FAULT)
//       {
//         OUT_DIAG_OnErrorFallback(id);
//       }
//     }
//     vPortExitCritical();
//   }
// }


// Performance watches
static volatile uint32_t DWToutBtsPROC = 0;
static volatile uint32_t DWToutBtsPER = 0;
static volatile uint32_t t1, t2 = 0;

// FAST CONTROL LOOP BODY
void OUT_DIAG_AllBts(void)
{
  t1 = DEBUG_ARM_GET_TIME;
  DWToutBtsPER = DEBUG_ARM_CLOCKS_TO_US(t1 - t2);
  for(uint8_t id = 0; id < OUT_ID_BTS_MAX; id++)
  {
    OUT_DIAG_SingleBtsNew(id);
  }
  t2 = DEBUG_ARM_GET_TIME;
  DWToutBtsPROC = DEBUG_ARM_CLOCKS_TO_US(t2 - t1);
}

/// @brief Perform all needed processing for output channel of SPOC2 type
/// @param id Output channel id [9..16] T_OUT_ID
static void OUT_DIAG_SingleSpoc(T_OUT_ID id)
{
  // TODO Requires major rework
  OUT_ASSERT_IN_RANGE(id);
  
  T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  T_OUT_REG* reg = OUT_GETREGPTR(id);
  ASSERT(reg);

  // SPOC2 ONLY
  if( outsCfg[id].type != OUT_TYPE_SPOC2)
  {
    return;
  }
  
  // TODO VMUX_GetValue in two seperate sections? Here and in bts (does it make sense?)
  reg->voltageMV = VMUX_GetValue(id);
  reg->emaVoltageMV = (reg->voltageMV * OUT_DIAG_EMA_VOLTAGE_ALPHA) + (reg->emaVoltageMV * (1 - OUT_DIAG_EMA_VOLTAGE_ALPHA));

  reg->currentMA = BSP_OUT_CalcCurrent(id);
  reg->emaCurrentMA = (reg->currentMA * OUT_DIAG_EMA_CURRENT_ALPHA) + (reg->emaCurrentMA * (1 - OUT_DIAG_EMA_CURRENT_ALPHA));
  //uint32_t batteryVoltage = VMUX_GetBattValue();
  //uint32_t dkilis = BSP_OUT_GetDkilis(id);
  
  if(reg->state == OUT_STATE_ON)
  {
    reg->status = OUT_STATUS_OK;
  }
  else if(reg->state == OUT_STATE_OFF)
  {
    reg->status = OUT_STATUS_OK;
  }
  else if(reg->state == OUT_STATE_ERR_LATCH)
  {
    reg->status = OUT_STATE_ERR_LATCH;
  }

  return;   
}

void OUT_DIAG_AllSpoc(void)
{
  for(uint8_t id = OUT_ID_BTS_MAX; id < OUT_ID_MAX; id++)
  {
    OUT_DIAG_SingleSpoc(id);
  }
}

uint32_t OUT_DIAG_GetCurrent(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->currentMA;
}

uint16_t OUT_DIAG_GetCurrent_pA(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->currentMA / 10;
}

uint16_t OUT_DIAG_GetEmaCurrent_pA(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->emaCurrentMA / 10;
}

uint32_t OUT_DIAG_GetVoltage(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->voltageMV;
}

uint32_t OUT_DIAG_GetEmaVoltage(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->emaVoltageMV;
}

T_OUT_STATUS OUT_DIAG_GetStatus(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->status;
}

T_OUT_STATE OUT_DIAG_GetState(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETREGPTR(id)->state;
}

#pragma endregion DIAG_SECTION

T_OUT_TYPE OUT_GetType(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETCFGPTR(id)->type;
}

char* OUT_GetName(T_OUT_ID id)
{
 OUT_ASSERT_IN_RANGE(id);
 return OUT_GETCFGPTR(id)->name;
}

T_OUT_MODE OUT_GetMode(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  return OUT_GETCFGPTR(id)->mode;
}

void OUT_ResetRegistersAll(void)
{
  for(uint8_t id = 0; id < OUT_ID_MAX; id++)
  {
    T_OUT_REG* reg = OUT_GETREGPTR(id);
    ASSERT(reg);
    reg->state = OUT_STATE_OFF;
    reg->status = OUT_STATUS_OK;
    reg->currentMA = 0;
    reg->voltageMV = 0;
    reg->emaCurrentMA = 0;
    reg->emaVoltageMV = 0;
    reg->safety.inRetrySequence = FALSE;
    reg->safety.errRetryCounter = 0;
    reg->safety.retryTimerCounter = 0;
    reg->safety.socReg.status = OUT_SAFETY_SOC_NORMAL_OPERATION;
    reg->safety.socReg.currentThreshold = 0;
    reg->safety.socReg.timeInPreInrush = 0;
    reg->safety.socReg.inrushTripCounter = 0;
    reg->safety.i2tReg.i2tSum = 0;
  }
}

inline bool OUT_IsSafetyLineDependent(T_OUT_ID id)
{
  OUT_ASSERT_IN_RANGE(id);
  const T_OUT_CFG* cfg = OUT_GETCFGPTR(id);
  ASSERT(cfg);

  return (cfg->safety.actOnSafety == true);
}


void testTaskEntry(void *argument)
{
  for(;;)
  {
    osDelay(10);
  }
  // /* TODO There is possibility to add UT here for setting output mode and setting output state */
  // T_OUT_CFG* out = OUT_GetCfgPtr( OUT_ID_1 );

  // OUT_ChangeMode( out, OUT_MODE_STD ); 
  // //OUT_SetState( out, OUT_STATE_ON );
  // for (;;)
  // {
  //   OUT_ToggleState( out );
  //   osDelay(3000);
  // }

  // SPOC2_Init();
  // OUT_ChangeMode(OUT_ID_1, OUT_MODE_PWM);
  // for(;;)
  // {
  //   for(uint8_t i = 0; i < 100; i++)
  //   {
  //     BSP_OUT_SetDutyPWM(OUT_ID_1, i);
  //     osDelay(10);
  //   }

  // }

  // ADC_ChannelConfTypeDef sConfig = {0};
  // sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
  // sConfig.Channel = ADC_CHANNEL_6;
  // sConfig.Rank = 2;
  // if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  // {
  //   Error_Handler();
  // }
  // HAL_ADC_Start(&hadc3);
  // HAL_ADC_PollForConversion(&hadc3, HAL_MAX_DELAY);
  // adc_val = HAL_ADC_GetValue(&hadc3);

  // for( T_OUT_ID i = OUT_ID_1; i < OUT_ID_MAX; i++)
  // {
  //   OUT_ChangeMode( i, OUT_MODE_STD);
  // }

  // for(;;)
  // {
  //   for( T_OUT_ID i = OUT_ID_1; i < OUT_ID_MAX; i++)
  //   {
  //     OUT_ToggleState(i);
  //   }
  //   osDelay(2000);
  //   //SPOC2_SelectSenseMux(SPOC2_ID_1, 0);
  // }
}

///
/// TODO [MAJOR REWORK] 
/// [DONE] Seperate output control code from safety functions, move safety functions into blocks, 
/// safety on seperate core or assure that it is safe to run always (on hardware timer as critical section), 
/// Status and state calculations should be done more properly
/// Add signal filtering on current and voltage signals
/// Handle PWM  signals [WORK IN PROGRESS]
/// [DONE] [IMPORTANT ]Add safety detection as status or state
/// [DONE] Assure safety retry correct working - does safety affect diagnosis? 
/// [DONE] Remove this 4x shit from OC trip
/// Sync current and voltage 
/// Change RTOS tasks names (remove entry nomencalture)
/// Read all and clean up defines
/// [DONE ]Unify calls to cfg and reg
/// Change time quanta to 1ms
/// [DONE] Added software oversampling to current detection // at 10kHz