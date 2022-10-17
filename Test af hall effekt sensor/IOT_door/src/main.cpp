#include <Arduino.h>

//pins
int onboardLED = 2;
int sensor = 15;

void setup() {
  Serial.begin(9600);

  //Setup pins
  pinMode(onboardLED, OUTPUT);
  pinMode(sensor, INPUT_PULLUP);

  digitalWrite(onboardLED, LOW);
  
}

void loop() {
  Serial.print("hall effect sensor: ");
  Serial.println(analogRead(sensor));
  delay(1000);
}
