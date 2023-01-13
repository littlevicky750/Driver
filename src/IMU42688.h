#ifndef IMU42688_h
#define IMU42688_h
#include <Arduino.h>
#include "WC_IMU.h"


#ifndef IMU_C
#define IMU_C 3
#endif

class IMU42688
{
private:
    float AngleMeasure[IMU_C][3];
    int IMUStart = 0;
    int CopeFailed = 0;

public:
    float Angle[3] = {0, 0, 0};
    float AngleShow[3] = {0, 0, 0};
    float SensorTemperature;
    byte Gravity = 2;
    byte GravityPrevious = 1;
    const byte IMU_Update_Success = 0;
    const byte Err_IMU_Not_Warm_Up = 1;
    const byte Err_IMU_Receive_Data_Error = 2;
    const byte Err_IMU_Data_StDev_Outside_TrustHold = 3;
    const byte Err_IMU_Cope_Failed = 4;
    byte ErrorCode = Err_IMU_Not_Warm_Up;
    byte fWarmUp;
    void Initialize(byte Rx /*(-1)*/, byte Tx /*(-1)*/);
    byte Update();
    float getHorizontal();
    float getVertical();
    float getHorizontalFilt();
    float getVerticalFilt();
};

#endif