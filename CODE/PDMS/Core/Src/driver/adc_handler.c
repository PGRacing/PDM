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
volatile static uint32_t DWTmedianStartTS = 0;
volatile static uint32_t DWTmedianPROC = 0;
volatile static uint32_t DWTadc1StartTS = 0;
volatile static uint32_t DWTadc1PROC = 0;
#endif

#define ADC1_MEDIAN_FILTER_WINDOW 3

static void insertionSort(uint16_t *arr, uint32_t len) {
    // Force 32-bit native registers for loop mechanics to avoid UXTH penalties
    for (uint32_t i = 1; i < len; i++) {
        uint32_t key = arr[i]; // Promoted to 32-bit register natively
        int32_t j = (int32_t)i - 1;
        
        // Optimize for ARM pipeline: comparison using 32-bit register values
        while (j >= 0 && (uint32_t)arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = (uint16_t)key;
    }
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
#ifdef DEBUG
            DWTadc1StartTS = DEBUG_ARM_GET_TIME;
#endif
            // Prepare adc data
#ifdef DEBUG
            DWTmedianStartTS = DEBUG_ARM_GET_TIME;
#endif
            for(uint8_t i = 0; i < ADC1_CHANNEL_COUNT; i++)
            {
                // Sort data at first because it will be averaged either way
                //qsort((uint16_t*)adc1MedianBufferShadow[i], ADC1_SW_OVERSAMPLING_RATIO, sizeof(uint16_t), compareUINT16);
                insertionSort((uint16_t*)&(adc1MedianBufferShadow[i][0]), ADC1_SW_OVERSAMPLING_RATIO);

                // Median filter
                for(int32_t j = 1; j < ADC1_SW_OVERSAMPLING_RATIO - 1; j++)
                {
                    int32_t start = j - (ADC1_MEDIAN_FILTER_WINDOW / 2);
                    int32_t end = j + (ADC1_MEDIAN_FILTER_WINDOW / 2);
                    
                    // Clamp start and end
                    if(start < 1)
                    {
                        start = 1;
                    }
                    if(end > ADC1_SW_OVERSAMPLING_RATIO - 2)
                    {
                        end = ADC1_SW_OVERSAMPLING_RATIO - 2;
                    }

                    int32_t center = (start + end) / 2;
                    uint16_t median = adc1MedianBufferShadow[i][center];

                    // Replace original sample with median value
                    adc1MedianBufferShadow[i][j] = median;
                }

                // Average oversampling values, after median filtering, to get final ADC value
                for(int32_t j = 1; j < ADC1_SW_OVERSAMPLING_RATIO - 1; j++)
                {
                    adc1SumData[i] += adc1MedianBufferShadow[i][j];
                }
                adc1AvgData[i] = adc1SumData[i] / (ADC1_SW_OVERSAMPLING_RATIO - 2);
                adc1SumData[i] = 0;
            }      
#ifdef DEBUG
            DWTmedianPROC = DEBUG_ARM_CLOCKS_TO_US(DEBUG_ARM_GET_TIME - DWTmedianStartTS);
#endif
            // Start channel diagnostics
            OUT_DIAG_AllBts();       
#ifdef DEBUG
            DWTadc1PROC = DEBUG_ARM_CLOCKS_TO_US(DEBUG_ARM_GET_TIME - DWTadc1StartTS);
#endif
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