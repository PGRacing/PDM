#include "adc_handler.h"
#include "adc.h"
#include "logger.h"
#include "out.h"
#include "typedefs.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "bsp_phyinput.h"
#include "semphr.h"
#include <stdlib.h>
#include <stdint.h>

xTaskHandle adc1TaskHandleLocal;
xTaskHandle adc2TaskHandleLocal;
SemaphoreHandle_t adc1ConvReadySemaphore = NULL;
SemaphoreHandle_t adc2ConvReadySemaphore = NULL;

volatile uint32_t adc1SumData[ADC1_CHANNEL_COUNT] = {0};
volatile uint16_t adc1AvgData[ADC1_CHANNEL_COUNT] = {0};

#ifdef DEBUG
volatile static uint32_t DWTmedianTS = 0;
volatile static uint32_t DWTmedianPROC = 0;
#endif

#define ADC1_MEDIAN_FILTER_WINDOW 3

int compareUINT16(const void *a, const void *b) {
    uint16_t arg1 = *(const uint16_t *)a;
    uint16_t arg2 = *(const uint16_t *)b;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

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
            // Prepare adc data
#ifdef DEBUG
            DWTmedianTS = DEBUG_ARM_GET_TIME;
#endif
            for(uint8_t i = 0; i < ADC1_CHANNEL_COUNT; i++)
            {
                uint16_t window[ADC1_MEDIAN_FILTER_WINDOW];

                for(int32_t j = 0; j < ADC1_SW_OVERSAMPLING_RATIO; j++)
                {
                    int32_t start = j - (ADC1_MEDIAN_FILTER_WINDOW / 2);
                    int32_t end = j + (ADC1_MEDIAN_FILTER_WINDOW / 2);
                    
                    // Clamp start and end
                    if(start < 0)
                    {
                    start = 0;
                    }
                    if(end > ADC1_SW_OVERSAMPLING_RATIO - 1)
                    {
                    end = ADC1_SW_OVERSAMPLING_RATIO - 1;
                    }

                    int32_t ws = 0;
                    for(int32_t k = start; k <= end; k++)
                    {
                    window[ws++] = adc1MedianBufferShadow[i][k];
                    }

                    // Sort window
                    qsort(window, ws, sizeof(uint16_t), compareUINT16);
                    uint16_t median = window[ws/2];

                    // Replace original sample with median value
                    adc1MedianBufferShadow[i][j] = median;
                }

                // Average oversampling values, after median filtering, to get final ADC value
                for(int32_t j = 0; j < ADC1_SW_OVERSAMPLING_RATIO; j++)
                {
                    adc1SumData[i] += adc1MedianBufferShadow[i][j];
                }
                adc1AvgData[i] = adc1SumData[i] / ADC1_SW_OVERSAMPLING_RATIO;
                adc1SumData[i] = 0;
            }      
#ifdef DEBUG
            DWTmedianPROC = DEBUG_ARM_CLOCKS_TO_US(DEBUG_ARM_GET_TIME - DWTmedianTS);
#endif
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