extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include "driver/rtc_io.h"
#include <AsyncMqttClient.h>
#include <WifiConnect.hpp>

// #define WIFI_SSID "NETGEAR19"
// #define WIFI_PASSWORD "flamefire2.4"

#define WIFI_SSID "Flamefire"
#define WIFI_PASSWORD "12345678"

// Digital Ocean MQTT Mosquitto Broker
//#define MQTT_HOST IPAddress(104, 248, 100, 190)
// For a cloud MQTT broker, type the domain name
#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883

#define MQTT_USERNAME "flamefire"
#define MQTT_PASSWORD "1674Lfgi"

// Test MQTT Topic
#define MQTT_PUB_TEST "Cafeteria"

//ISR pin
#define SLEEP_PIN 27

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
WifiConnect wifi;
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool DoorClosed = false;

int i = 0;

unsigned long SetupBegin;
unsigned long SetupEnd;

//helper
void PrintInfo(uint16_t packetIdPub1)
{
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEST, packetIdPub1);
  Serial.printf(" Message: ");
  Serial.printf(String(DoorClosed).c_str());
  i++;
}

void WakeUpRoutine()
{
  DoorClosed = digitalRead(SLEEP_PIN);
  String payload = String(bootCount) + "." + String(DoorClosed);
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEST, 1, true, payload.c_str());                            
  PrintInfo(packetIdPub1);
}

void connectToWifi() {
  wifi.BeginWiFiConnection();
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  WakeUpRoutine();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  SetupEnd = micros();
  Serial.print("\nGoing to sleep now \nTotal time alive :");
  Serial.print((SetupEnd - SetupBegin)/1000);
  Serial.print("ms\n");
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  SetupBegin = 0;
  SetupEnd = 0;

  SetupBegin = micros();

  pinMode(SLEEP_PIN, INPUT);

  rtc_gpio_pullup_en(GPIO_NUM_27);

  if (DoorClosed) { 
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, false); 
  }
  else { 
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, true); 
  }

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  /*mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);*/
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  //mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD); //authentication
  connectToWifi();
}

void loop() { }

