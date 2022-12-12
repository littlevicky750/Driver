#ifndef OnOff_H
#define OnOff_H
#include <Arduino.h>
#include "MotorDriver.h"
#include "SDCard.h"
#include "SerialDebug.h"
#include "OLED.h"
#include "RGBLED.h"


RTC_DATA_ATTR int bootCount = -1;

class OnOff
{
private:
    byte ButPin;
    OLED oledb;
    int OffClock = 0;

public:
    SDCard *pSD;
    MotorDriver *pMD;
    void On(gpio_num_t WakeUpPin)
    {
        ButPin = WakeUpPin;
        LED.SetUp();
        oledb.PowerSave(1);
        esp_sleep_enable_ext0_wakeup(WakeUpPin, 0);

        bootCount++;
        // Begine from sleeping
        if (bootCount == 0)
        {
            esp_deep_sleep_start();
        }
        pinMode(ButPin, INPUT);
        LED.Write(2, 255, 255, 255, LED.LIGHT30);
        LED.Update();

        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
        {
            // Detect 3 min long press
            while (millis() < 1000 && digitalRead(ButPin) == 0)
            {
                delay(100);
            }
            LED.Off();
            if (millis() < 1000)
            {
                esp_deep_sleep_start();
            }
        }
        oledb.Initialize();
        while (digitalRead(ButPin) == 0)
            delay(100);
        Serial.begin(115200);
        Serial.print("wake up ");
        Serial.println(bootCount);
        
    }

    void Off_Clock_Start()
    {
        OffClock = millis();
    }

    void Off_Clock_Stop()
    {
        OffClock = 0;
    }

    void OffCheck()
    {
        if (OffClock == 0)
        {
            return;
        }
        if (millis() - OffClock > 3000)
        {
            cli();
            if (pMD)
                pMD->Emergency_Stop(1);
            Debug.println("Function Off");
            if (pSD)
            {
                String T = "";
                pSD->Save("", T);
            }
            OLED oledb;
            oledb.PowerSave(1);
            detachInterrupt(digitalPinToInterrupt(ButPin));
            LED.Off();
            LED.Write(2, 255, 255, 255, LED.LIGHT30);
            LED.Update();
            while (digitalRead(ButPin) == 0)
            {
                delay(100);
            }
            LED.Off();
            esp_deep_sleep_start();
        }
        else
        {
            // LED.Write(2,255,255,255,LED.LIGHT30);
        }
    }
};

#endif