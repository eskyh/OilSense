#include <Arduino.h>
#include "EspClient.hpp"

// Create a singleton instance of EspClient.
EspClient &espClient = EspClient::instance();

void setup()
{
  //-- Initialize MCU Pins and Serial speed

  // Initialize digital pin LED_BUILTIN as an output (used as ESP heartbeat indicator).
  pinMode(LED_BUILTIN, OUTPUT);
  // Turn the LED on (LOW) or off (HIGH) as needed. (NOTE: It is inverted.)    
  digitalWrite(LED_BUILTIN, HIGH); 

  //-- Serial Communication baudrate options: 9600, 115200, 250000
  Serial.begin(115200);            

  //-- EspClient config and connection establishment:
  //   WiFi, MQTT, OTA, Init Sensors, etc.
  espClient.setup();
}

void loop()
{
  //-- Run EspClient tasks:
  // sensor measurement and publishing, web portal handling,
  // actuator MQTT commands, and Wi-Fi/MQTT reconnections.
  espClient.loop();
}
