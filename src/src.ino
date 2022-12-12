const gpio_num_t IO_Extern_Wakeup = GPIO_NUM_0;
const byte IO_Button_SW = GPIO_NUM_0;
const byte IO_MD_I_FB = 4; // Analog Input, motor feedback current.
const byte IO_MD_V_FB = 7; //
const byte IO_MD_Dir = 6;  // OutPut, PH
const byte IO_MD_V = 16;   // Output, EN
const byte IO_MD_Swich = 18;
const byte IO_MD_Brak = 48; // Output, LOW to wake up, HIGH to sleep.
const byte IO_SD_MOSI = 12;
const byte IO_SD_MISO = 13;
const byte IO_SD_SCK = 14;
const byte IO_SD_CS = 15;
const byte IO_Battery = 17;
const byte IO_Button_CL = 40;
const byte IO_Button_DT = 41;
#define LED_PIN 42

// IMU with Serial0
#include "MotorDriver.h"
#include "SDCard.h"
#include "OLED.h"
#include "Clock.h"
#include "OnOff.h"
#include "Battery.h"

#include "SerialDebug.h"
SerialDebug Extern_Serial_Debug;
extern SerialDebug Debug = Extern_Serial_Debug;
RGBLED Extern_RGB_LED;
extern RGBLED LED = Extern_RGB_LED;

byte Page = 2;
byte Cursor = 0;

MotorDriver MD;
OLED oled;
Clock L_clock;
OnOff Swich;
SDCard sdCard;
Battery battery;

#include "Net.h"
#include "Button.h"

TaskHandle_t T_LOOP;
TaskHandle_t T_FAST;
TaskHandle_t T_CONN;
TaskHandle_t T_SEND;
TaskHandle_t T_MDCON;
TaskHandle_t T_CHECK;
TaskHandle_t T_SAVE;
TaskHandle_t T_BACK;

String MD_Last_Recieve;

static void Conn(void *pvParameter)
{
  Server_Initialize();
  Client_Initialize();
  for (;;)
  {
    Client_Connect();
    vTaskDelay(3000);
  }
}

static void Send(void *pvParameter)
{
  vTaskDelay(10000);
  for (;;)
  {
    BLE_Manual_Command_Check();
    BLE_Send_Update_Angle();
    vTaskDelay(50);
  }
}

static void Check(void *pvParameter)
{
  for (;;)
  {
    vTaskDelay(30 * 60 * 1000);
    Check_Server_Characteristic();
  }
}

static void MDCON(void *pvParameter)
{
  for (;;)
  {
    MD.Check_Connect();
    MD.AccControl();
    vTaskDelay(1000);
    // vTaskDelay(StepTime / 5);
  }
}

static void Loop(void *pvParameter)
{
  for (;;)
  {
    
    Swich.OffCheck();
    LED.Update();
    MD_Last_Recieve = L_clock.toString(LastUpdate);
    vTaskDelay(500);
  }
}

static void Fast(void *pvParameter)
{
  delay(1000);
  for (;;)
  {
    oled.Update();
    Button_Update();
    vTaskDelay(50);
  }
}

static void Back(void *pvParameter)
{
  for (;;)
  {
    battery.Update();
    MD.CurrentFB();
    vTaskDelay(3);
  }
}

static void Save(void *pvParameter)
{
  vTaskDelay(5000);
  sdCard.SetPin(IO_SD_SCK, IO_SD_MISO, IO_SD_MOSI, IO_SD_CS);
  for (;;)
  {
    String Msg = String(millis());
    sdCard.Save("/Motor_Driver_SD_Test", Msg);
    vTaskDelay(5*60000);
  }
}

void setup()
{
  Swich.On(GPIO_NUM_0);
  Swich.pSD = &sdCard;
  Swich.pMD = &MD;
  Debug.Setup(sdCard);
  Debug.printOnTop("-------------------------ESP_Start-------------------------");
  MD.Initialize(IO_MD_V, IO_MD_Dir, IO_MD_Brak, IO_MD_V_FB, IO_MD_I_FB, IO_MD_Swich);
  battery.SetPin(IO_Battery);
  oled.pMD = &MD;
  oled.Battery = &battery.Percent;
  oled.isConnect = &isConnect;
  oled.isAdvertising = &keepAdvertising;
  oled.isScanning = &isScanning;
  oled.Address = &Address;
  oled.pMD_C_show = &MD_Last_Recieve;
  oled.Angle = &Angle;
  xTaskCreatePinnedToCore(MDCON, "MDCont", 2048, NULL, 5, &T_MDCON, 1);
  xTaskCreatePinnedToCore(Fast, "Fast", 10000, NULL, 2, &T_FAST, 1);
  xTaskCreatePinnedToCore(Loop, "Main", 10000, NULL, 3, &T_LOOP, 1);
  xTaskCreatePinnedToCore(Back, "BackGround", 10000, NULL, 1, &T_BACK, 1);
  xTaskCreatePinnedToCore(Conn, "Conn", 20000, NULL, 3, &T_CONN, 0);
  xTaskCreatePinnedToCore(Send, "Send", 10000, NULL, 2, &T_SEND, 0);
  //xTaskCreatePinnedToCore(Save, "Save", 10000, NULL, 2, &T_SAVE, 0);
  xTaskCreatePinnedToCore(Check, "Check Char", 10000, NULL, 1, &T_CHECK, 0);

  Button_Iitialize();
}

void loop()
{
}