#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include "Arduino.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

#define EEPROM_SIZE 12

//Data that is stored
int timeForCoffeeHH;          //0x0
int timeForCoffeeMM;          //0x1
bool AlarmOn = false;         //0x2
bool CoffeeMachineOn = false; //0x3
int TimeForCoffeeOffMM;       //0x4
bool ActiveWeekdays[7];       //0x5 - 0x11
int CoffeeMachineNotOff = 0;  //0x12

const char* ssid = "Stofa24867"; 
const char* password =  "stile13fancy15";

//NTP server request time data
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String weekDays[7]={};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

AsyncWebServer server(80);

int TransistorBasePin = 15;
int OnBoardLED = 2;

const char* input_parameter1 = "hh";
const char* input_parameter2 = "mm";
const char* input_parameter[7] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
String inputMessage1;
String inputMessage2;

//FreeRTOS counters and tasks
int count2 = 0;
int count4 = 0;
bool AlarmFlag = false;

TaskHandle_t task2Handle = NULL;
TaskHandle_t task3Handle = NULL;
TaskHandle_t task4Handle = NULL;
eTaskState taskState;

bool CheckAlarm();
void CoffeeAlarm();
void TurnOnMachine();
void TurnOffMachine();
void task2(void* parameters); //Task waits for interupt
void task3(void* parameters); //Task waits for interupt
void task4(void* parameters); //Task gets run in setup, to check for unfinished business

void setup()
{
  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("EEPROM failed to initialise");
    while (true);
  }
  else
  {
    Serial.println("EEPROM initialised");
  }
  
  //Setup pins
  pinMode(OnBoardLED, OUTPUT);
  pinMode(TransistorBasePin, OUTPUT);
  digitalWrite(TransistorBasePin, LOW);
 
  //Connect to wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    digitalWrite(OnBoardLED, HIGH);
    delay(1000);
    digitalWrite(OnBoardLED, LOW);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  
  //Setup NTP time
  timeClient.begin(); 
  timeClient.setTimeOffset(7200);

  //Send a GET request to <ESP_IP>/SetWeekdays?monday=<inputMessage1>&tuesday=<inputMessage2>...
  server.on("/SetWeekdays", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    if (request->hasParam(input_parameter[0]) &&
        request->hasParam(input_parameter[1]) &&
        request->hasParam(input_parameter[2]) &&
        request->hasParam(input_parameter[3]) &&
        request->hasParam(input_parameter[4]) &&
        request->hasParam(input_parameter[5]) &&
        request->hasParam(input_parameter[6]))
    {
      for (size_t i = 0; i < 7; i++)
      {
        //Serial.println(i + ":" + request->getParam(input_parameter[i])->value());
        if (request->getParam(input_parameter[i])->value() == "true")
        {
          ActiveWeekdays[i] = 1;
        }
        else
        {
          ActiveWeekdays[i] = 0;
        }
      }
      EEPROM.write(5,ActiveWeekdays[0]);
      EEPROM.write(6,ActiveWeekdays[1]);
      EEPROM.write(7,ActiveWeekdays[2]);
      EEPROM.write(8,ActiveWeekdays[3]);
      EEPROM.write(9,ActiveWeekdays[4]);
      EEPROM.write(10,ActiveWeekdays[5]);
      EEPROM.write(11,ActiveWeekdays[6]);
      EEPROM.commit();
      vTaskDelay(2000);
    }
    else 
    {
      Serial.print("No weekdays received, or missing data");
    }
    Serial.print("monday = ");
    Serial.println(ActiveWeekdays[0]);
    Serial.print("tuesday= ");
    Serial.println(ActiveWeekdays[1]);
    Serial.print("wednesday = ");
    Serial.println(ActiveWeekdays[2]);
    Serial.print("thursdag= ");
    Serial.println(ActiveWeekdays[3]);
    Serial.print("friday = ");
    Serial.println(ActiveWeekdays[4]);
    Serial.print("saturday= ");
    Serial.println(ActiveWeekdays[5]);
    Serial.print("sunday= ");
    Serial.println(ActiveWeekdays[6]);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/SetTimer?hh=<inputMessage1>&mm=<inputMessage2>
  server.on("/SetTimer", HTTP_GET, [](AsyncWebServerRequest *request) 
  {    
    // GET input1 value on <ESP_IP>/SetTimer?hh=<inputMessage1>&mm=<inputMessage2>
    if (request->hasParam(input_parameter1) && request->hasParam(input_parameter2) && !CoffeeMachineOn) 
    {
      inputMessage1 = request->getParam(input_parameter1)->value();
      inputMessage2 = request->getParam(input_parameter2)->value();
      if (EEPROM.read(2) != 255)
      {
        vTaskSuspend(task3Handle);
        // if (eTaskGetState(task3Handle) == eTaskState::eRunning)
        // {
        //   vTaskSuspend(task3Handle);
        // }
      }
      

      //Data
      timeForCoffeeHH = inputMessage1.toInt();
      timeForCoffeeMM = inputMessage2.toInt();
      AlarmOn = true;
      AlarmFlag = true;

      //Save data to EEPROM
      EEPROM.write(0, timeForCoffeeHH);
      EEPROM.write(1, timeForCoffeeMM);
      EEPROM.write(2, AlarmOn);
      EEPROM.commit();
      vTaskDelay(2000);

      // Serial.print("Address : 0 =");
      // Serial.println(EEPROM.read(0));
      // Serial.print("Address : 1 =");
      // Serial.println(EEPROM.read(1));
      // Serial.print("Address : 2 =");
      // Serial.println(EEPROM.read(2));

      
      // Serial.print(inputMessage1.toInt());
      // Serial.print(":");
      // Serial.println(inputMessage2.toInt());
    }
    else 
    {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  }); 

  //Coffee on / Coffee off
  server.on("/coffeeBtn", HTTP_GET   , [](AsyncWebServerRequest *request)
  {
    CoffeeMachineOn = !CoffeeMachineOn;
    if(CoffeeMachineOn)
    {
      request->send(200, "text/plain", "ON");
      Serial.println("Coffee Machine ON");
    }
    if(!CoffeeMachineOn)
    {
      request->send(200, "text/plain", "OFF");
      Serial.println("Coffee Machine OFF");
    }

    digitalWrite(TransistorBasePin, HIGH);
    delay(1000);
    digitalWrite(TransistorBasePin, LOW);
  });
  
  //Status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/plain", String(CoffeeMachineOn));
  });

  Serial.print("Alarm on:   ");
  Serial.println(EEPROM.read(2));
  Serial.print("Machine on: ");
  Serial.println(EEPROM.read(3));

  if (EEPROM.read(2) == 1 && EEPROM.read(3) == 0)
  {
    Serial.println("Creating task3");

    xTaskCreate
    (
      task3,         //function name
      "Coffee Alarm",     //task name
      8191,          //stack size
      NULL,          //task parameters
      1,             //task priority
      &task3Handle   //task handle
    );
  }

  if (EEPROM.read(2) == 1 && EEPROM.read(3) == 1)
  {
    // if (EEPROM.read(12) > 1)
    // {   
    //   Serial.println("CoffeeMachineNotOff > 1");
    //   TurnOffMachine();
    // }
    // else
    // {
    //   EEPROM.write(12, CoffeeMachineNotOff++);
    //   EEPROM.commit();
    //   delay(1000);
    //   Serial.println("Creating task2, addr12: " + EEPROM.read(12));
    //Start 15min countdown to turnoff
    xTaskCreate
    (
    task2,         //function name
    "Countdown to turnoff",     //task name
    8191,          //stack size
    NULL,          //task parameters
    1,             //task priority
    &task2Handle   //task handle
    );
    //}
  }
  
  server.begin();
}

void loop(){
  if (AlarmFlag)
  {
    AlarmFlag = false;
    CoffeeAlarm();
  }
}


//***************************** Functions *****************************//

void CoffeeAlarm()
{
  xTaskCreate
  (
    task3,            //function name
    "Coffee Alarm",   //task name
    8191,             //stack size
    NULL,             //task parameters
    tskIDLE_PRIORITY, //task priority
    &task3Handle      //task handle
  );
}

//Task for checking alarm
void task3(void* parameters)
{
  for(;;)
  {
    if (CheckAlarm() && task3Handle != NULL)
    {
      TurnOnMachine();
      
      xTaskCreate
      (
        task2,            //function name
        "Countdown to turnoff",     //task name
        8191,             //stack size
        NULL,             //task parameters
        tskIDLE_PRIORITY, //task priority
        &task2Handle      //task handle
      );  
      vTaskSuspend(task3Handle);
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

//Task for counting 15min
void task2(void* parameters)
{
  for(;;)
  {
    if (count2 > 300 && task2Handle != NULL || CoffeeMachineOn == false)
    {
      TurnOffMachine();

      xTaskCreate
      (
        task3,            //function name
        "Countdown to coffee",     //task name
        8191,             //stack size
        NULL,             //task parameters
        tskIDLE_PRIORITY, //task priority
        &task3Handle      //task handle
      );
      count2 = 0;
      vTaskSuspend(task2Handle);
    }
    Serial.print("Task2 counter: ");
    Serial.println(count2++);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

bool CheckAlarm()
{
  //compare the timer hours and minutes with real time and days of the week from the saved EEPROM DATA
  timeClient.update();
  if(timeClient.getMinutes() == EEPROM.read(1) && timeClient.getHours() == EEPROM.read(0))
  {
    for (int i = 1; i < 8; i++)
    {
      Serial.print("i: ");
      Serial.println(i);
      Serial.print("Day: ");
      Serial.println(timeClient.getDay());
      Serial.print("Alarm Day: ");
      Serial.println(EEPROM.read(i+4));
      if (timeClient.getDay() == i && EEPROM.read(i+4) == 1)
      {
        return true;
      }
    }
  }
  Serial.print("Current hour(");
  Serial.print(timeClient.getHours());
  Serial.print(")   != Alarm hour(");
  Serial.print(EEPROM.read(0));
  Serial.println(")");

  Serial.print("Current Minute(");
  Serial.print(timeClient.getMinutes());
  Serial.print(") != Alarm Minutes(");
  Serial.print(EEPROM.read(1));
  Serial.println(")");
  return false;
}

void TurnOffMachine()
{
    //Turnoff coffee machine after 15min
    digitalWrite(TransistorBasePin, HIGH);
    delay(1000);
    digitalWrite(TransistorBasePin, LOW);
    Serial.println("Coffee Machine OFF");
    CoffeeMachineOn = false;  
    EEPROM.write(3, CoffeeMachineOn);
    EEPROM.commit();
}

void TurnOnMachine()
{
    //Start making coffee
    digitalWrite(TransistorBasePin, HIGH);
    delay(1000);
    digitalWrite(TransistorBasePin, LOW);
    Serial.println("Coffee Machine ON");
    CoffeeMachineOn = true;
    EEPROM.write(3, CoffeeMachineOn);
    EEPROM.commit();
}





