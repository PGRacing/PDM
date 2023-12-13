#include "main.h"
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "typedefs.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "adc.h"
#include "cmsis_os2.h"

// DIAGNOSIS REGISTERS

// SPOC2 Warning diagnosis register
#define SPOC2_WRNDIAG 0001
// SPOC2 Standard diagnosis register
#define SPOC2_STDDIAG 0010
// SPOC2 Error diagnosis register
#define SPOC2_ERRDIAG 0011

/// CONTROL REGISTERS

// SPOC2 Output configuration register
#define SPOC2_OUT 0000
// SPOC2 Restart counter status register (RO)
#define SPOC2_RCS 1000
// SPOC2 Slew rate control register (RO)
#define SPOC2_SRC 1001
// SPOC2 Overcurrent control register
#define SPOC2_OCR 0100
// SPOC2 Restart counter disable register
#define SPOC2_RCD 1100

