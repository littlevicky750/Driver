#include "MotorDriver.h"
#include "RGBLED.h"
#include "Wire.h"

MotorDriverSpeed MDSpeed;

void IRAM_ATTR MD_Speed_ISR()
{
    unsigned int Te = micros();
    if (MDSpeed.last_Rising == 0) // after reset
    {
        MDSpeed.last_Rising = Te;
    }
    else if (Te < MDSpeed.last_Rising || Te - MDSpeed.last_Rising > 500000) // if micros() reset or motor reset
    {
        MDSpeed.last_Rising = 0; // Don't count
    }
    else if (Te - MDSpeed.last_Rising > 1000)
    {
        MDSpeed.Period[MDSpeed.C] = Te - MDSpeed.last_Rising;
        MDSpeed.last_Rising = Te;
        MDSpeed.C++;
        MDSpeed.C %= 200;
    }
}

void MotorDriver::Initialize(byte IO_V, byte IO_Dir, byte IO_Brak, byte IO_V_FB, byte IO_I_FB, byte IO_SW)
{
    MD_Brak = IO_Brak;
    MD_Dir = IO_Dir;
    MD_I_FB = IO_I_FB;
    MD_V = IO_V;
    SpeedFB = &MDSpeed;
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
    ledcAttachPin(IO_V, PWM_CHANNEL);
    pinMode(IO_Dir, OUTPUT);
    pinMode(IO_V, OUTPUT);
    pinMode(IO_Brak, OUTPUT);
    pinMode(IO_V_FB, INPUT);
    pinMode(IO_I_FB, INPUT);
    Swich = digitalRead(IO_SW);
    digitalWrite(IO_Brak, LOW);
    digitalWrite(MD_Dir, LOW);
    attachInterrupt(digitalPinToInterrupt(IO_V_FB), MD_Speed_ISR, RISING);
}

void MotorDriver::Check_Connect()
{
    if (Check || !Swich)
        return;
    analogWrite(MD_V, 10);
    delay(200);
    if (SpeedFB->F_Hz() != -1)
    {
        Check = true;
        digitalWrite(MD_Dir, HIGH);
        analogWrite(MD_V, 10);
        delay(150);
    }
    analogWrite(MD_V, 0);
}

void MotorDriver::AccControl()
{
    Update_Feedback();
    if (Vc != 0)
    {
        u_out = Vc * VrtoVl / 4.0 + 0.5;
        /*
        if (Speed == -1)
        {
            u_out = Vc * VrtoVl / 4.0 + 0.5;
        }
        else if (Vc > 0)
        {
            u_out += ((abs(Vc) * VrtoVl - Speed) / 4.0 * 0.3 + 0.5);
        }
        else
        {
            u_out -= ((abs(Vc) * VrtoVl - Speed) / 4.0 * 0.1 + 0.5);
        }
        */
        u_out = max(min(255, u_out), -255);
    }
    if (!Swich)
    {
        LED.Write(2, 256, 0, 0, LED.LIGHT30);
        u_out = 0;
    }
    else if (!Check)
    {
        LED.Write(2, 256, 0, 0, LED.BLINK30);
        u_out = 0;
    }
    else if (u_out == 0)
    {
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    if (u_out != u_t0)
    {
        if (u_out == 0)
            digitalWrite(MD_Brak, HIGH);
        else if (u_t0 == 0)
            digitalWrite(MD_Brak, LOW);

        int u_t1 = u_out; //*(4095/255);

        // Max Acceleration control
        /*
        if (u_t1 > u_t0 + Max_Acc)
        {
            u_t1 = u_t0 + Max_Acc;
        }
        else if (u_t1 < u_t0 - Max_Acc)
        {
            u_t1 = u_t0 - Max_Acc;
        }
        */

        if (u_t1 > 0)
        {
            digitalWrite(MD_Dir, HIGH);
        }
        else
        {
            digitalWrite(MD_Dir, LOW);
        }
        // ledcWrite(PWM_MD, abs(u_t1));
        analogWrite(MD_V, abs(u_t1));
        if (abs(u_t0 - u_t1) > 10)
        {
            SpeedFB->Period[SpeedFB->C] = 0;
            SpeedFB->C++;
            SpeedFB->C %= 200;
        }
        u_t0 = u_t1;
    }
}

bool MotorDriver::Output(float AngularVelocity)
{
    LED.Write(2, 0, 0, 256, LED.BLINK30);
    Vc = -AngularVelocity * H * cos(MountedAngle); // if MountedAngle is a constant. (Assume Angle[0] ~ 90)
    // float LinearVelocity = - AngularVelocity * H * sin(Angle[0]*PI/180.0 - MountedAngle); // if MountedAngle can be measure
    if (Vc == 0 || abs(Vc) < MinimumVelocity)
    {
        u_out = 0;
        Vc = 0;
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    else if (Vc < 0)
    {
        Vc = (Vc < -MaximumVelocity) ? -MaximumVelocity : Vc;
        // u_out = LinearVelocity * 32.5 - 2;
        // u_out = LinearVelocity * 8 - 10;
    }
    else
    {
        Vc = (Vc > -MaximumVelocity) ? MaximumVelocity : Vc;
        // u_out = LinearVelocity * 32.5 + 2;
        // u_out = LinearVelocity * 8 + 11;
    }
    return 1;
}

bool MotorDriver::Manual(double Speed)
{
    LED.Write(2, 128, 128, 0, LED.BLINK30);
    if (Speed == 0)
    {
        u_out = 0;
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    else if (Speed < 0)
        u_out = -(255 - 10) * min(1.0, abs(Speed)) - 10;
    else if (Speed > 0)
        u_out = (255 - 10) * min(1.0, abs(Speed)) + 10;
    return 1;
}

void MotorDriver::CurrentFB()
{
    if (CFB_StartTime == 0)
    {
        CFB_Sum = analogRead(MD_I_FB);
        CFB_Count = 1;
        CFB_StartTime = millis();
    }
    else if (millis() - CFB_StartTime > 1000)
    {
        Current = CFB_Sum / CFB_Count;
        CFB_Sum = analogRead(MD_I_FB);
        CFB_Count = 1;
        CFB_StartTime = millis();
    }
    else
    {
        CFB_Sum += analogRead(MD_I_FB);
        CFB_Count++;
    }
}

void MotorDriver::Emergency_Stop(bool isStop)
{
    if (isStop)
    {
        Swich = false;
        AccControl();
    }
    else
    {
        Swich = true;
    }
}

void MotorDriver::Update_Feedback()
{
    if (SpeedFB->last_Rising == 0)
    {
        Speed = -1;
    }
    else if (micros() - SpeedFB->last_Rising > 500000)
    {
        SpeedFB->last_Rising = 0;
        Speed = -1;
    }
    else
    {
        int C = (SpeedFB->C + 199) % 200;
        int n = 0;
        int Pn = SpeedFB->Period[C % 200]; // Period(C-n)
        int Ps = 0;                        // Period(C) + Period(C-1) + ... + Period(C-n)
        while (Ps < 200000 && n < 200 && Pn != 0)
        {
            Ps += Pn;
            n++;
            Pn = SpeedFB->Period[(C + 200 - n) % 200];
        }
        Speed = (n != 0) ? 1 / (Ps / 1000000.0 / n) : -1;
    }
}