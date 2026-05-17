#include "adc_handler.h"
#include "adc.h"
#include "logger.h"
#include "out.h"
#include "typedefs.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "bsp_phyinput.h"
#include "semphr.h"

xTaskHandle adc1TaskHandleLocal;
xTaskHandle adc2TaskHandleLocal;
SemaphoreHandle_t adc1ConvReadySemaphore = NULL;
SemaphoreHandle_t adc2ConvReadySemaphore = NULL;

void ADCH_Init(void)
{
    adc1ConvReadySemaphore = xSemaphoreCreateBinary();

    if(adc1ConvReadySemaphore == NULL)
    {
        /* Error creating semaphore -> heap too small [?] */
        ASSERT(NULL);
    }

    ADC1_Init();

    adc2ConvReadySemaphore = xSemaphoreCreateBinary();
    
    if(adc2ConvReadySemaphore == NULL)
    {
        /* Error creating semaphore -> heap too small [?] */
        ASSERT(NULL);
    }
    
    ADC2_Init();
}

void adc1TaskStart(void *argument)
{
    adc1TaskHandleLocal = xTaskGetCurrentTaskHandle();
    LOG_INFO("ADC1:: Task start");
   
    for(;;)
    {
        /* Do something with data */
        if(xSemaphoreTake( adc1ConvReadySemaphore, portMAX_DELAY ) == pdTRUE)
        {
            // Start channel diagnostics
            OUT_DIAG_AllBts();       
        }
    }
}

void adc2TaskStart(void *argument)
{
    adc2TaskHandleLocal = xTaskGetCurrentTaskHandle();
    LOG_INFO("ADC2:: Task start");

    for(;;)
    {
        /* Do something with data */
        if(xSemaphoreTake( adc2ConvReadySemaphore, portMAX_DELAY ) == pdTRUE)
        {
            // Perform physical inputs assesment
            BSP_PHYIN_EvaluateValues();
        }
    }
}