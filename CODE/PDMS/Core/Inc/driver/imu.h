#ifndef __IMU_H_
#define __IMU_H_

#include "typedefs.h"
#include "math.h"

typedef struct _T_IMU_DATA_ACC
{
    float_t x;
    float_t y;
    float_t z;
}
T_IMU_DATA_ACC;

typedef struct _T_IMU_DATA_RATE
{
    float_t pitch;
    float_t roll;
    float_t yaw;
}
T_IMU_DATA_RATE;

void IMU_Init(void);
void IMU_GetAcceleration(T_IMU_DATA_ACC* acc);
void IMU_GetRates(T_IMU_DATA_RATE* rate);
float_t IMU_GetTemperature(void);

#endif // __IMU_H_