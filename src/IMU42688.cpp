#include "IMU42688.h"

WC_IMU IMU;

void IMU42688::Initialize(byte Rx /*(-1)*/, byte Tx /*(-1)*/)
{
    Serial1.setRxBufferSize(256);
    Serial1.begin(921600, SERIAL_8N1, Rx, Tx);
    IMUStart = millis();
    memset(AngleMeasure, 0, sizeof(AngleMeasure));
}

byte IMU42688::Update()
{
    // read multiple time in case of wrong coping
    for (int j = 0; j < IMU_C * 2; j++)
    {
        // Read Data from Serial1 --------------------------
        CopeFailed++;
        // Serial.print("Reading");
        int StartTime = millis();
        while (Serial1.available() && millis() - StartTime < 1000 && !IMU.newData)
        {
            // Serial.print(".");
            IMU.readSerialData(Serial1.read());
        }
        // Serial.println("");

        float AngleCope[6] = {0};
        byte Gravity_cope = 0;

        // Check if Got New Data -------------------------------------
        if (IMU.newData)
        {
            CopeFailed = 0;
            IMU.parseData();
        }
        else
        {
            ErrorCode = CopeFailed - 1 + Err_IMU_Cope_Failed;
            goto NextLoop;
        }

        // Read Angle and do basic check -----------------------------

        Gravity_cope = IMU.RecievedIMUData[7];
        if (Gravity_cope < 0 || Gravity_cope > 6)
        {
            ErrorCode = Err_IMU_Receive_Data_Error;
            goto NextLoop;
        }
        for (size_t i = 0; i < 6; i++)
        {
            AngleCope[i] = IMU.RecievedIMUData[i];
            if (AngleCope[i] == 0 || abs(AngleCope[i]) > 180)
            {
                ErrorCode = Err_IMU_Receive_Data_Error;
                goto NextLoop;
            }
        }

        memmove(&Angle[0], &AngleCope[0], sizeof(Angle));
        memmove(&AngleShow[0], &AngleCope[3], sizeof(Angle));
        Gravity = Gravity_cope;
        if(Gravity_cope == 4 || Gravity_cope ==1)
            GravityPrevious = Gravity_cope;
        SensorTemperature = IMU.RecievedIMUData[6];
        ErrorCode = IMU_Update_Success;
        /*
        Serial.print("X : ");
        Serial.print(AngleShow[0], 3);
        Serial.print(", Y : ");
        Serial.print(AngleShow[1], 3);
        Serial.print(", Z : ");
        Serial.print(AngleShow[2], 3);
        Serial.print(", g : ");
        Serial.print(", X : ");
        Serial.print(Angle[0], 3);
        Serial.print(", Y : ");
        Serial.print(Angle[1], 3);
        Serial.print(", Z : ");
        Serial.print(Angle[2], 3);
        Serial.print(", g : ");
        Serial.println(Gravity);
        //*/
        break;
    NextLoop:
        delay(10);
    } // end for

    // Check Warm Up Time ------------------------------
    if (fWarmUp != 100 && millis() - IMUStart < 60 * 1000)
    {
        ErrorCode = Err_IMU_Not_Warm_Up;
        fWarmUp = (millis() - IMUStart) / 600;
    }
    else if (fWarmUp != 100)
    {
        fWarmUp = 100;
    }
    return ErrorCode;
} // end void Update()

float IMU42688::getHorizontal()
{
    float H = Angle[1];
    if (Gravity == 1 || Gravity == 4)
    {
        H = - Angle[2];
    }
    return H;
}

float IMU42688::getVertical()
{
    float V = Angle[0];
    if (Gravity == 0 || Gravity == 3)
    {
        V = 90 - Angle[2];
        if (V > 180)
            V -= 360;
        else if (V < -180)
            V += 360;
    }
    return V;
}

float IMU42688::getHorizontalFilt()
{
    float H = AngleShow[1];
    if (Gravity == 1 || Gravity == 4)
    {
        H = - AngleShow[2];
    }
    return H;
}

float IMU42688::getVerticalFilt()
{
    float V = AngleShow[0];
    if (Gravity == 0 || Gravity == 3)
    {
        V = 90 - AngleShow[2];
        if (V > 180)
            V -= 360;
        else if (V < -180)
            V += 360;
    }
    return V;
}