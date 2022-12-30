#include <Arduino.h>
#include "EspClient.hpp"

#include <StensTimer.h> // https://gitlab.com/arduino-libraries/stens-timer

// Singleton EspClient instance
EspClient &espClient = EspClient::instance();

void setup()
{
  //-- Initialize Pins and Serial speed
  pinMode(LED_BUILTIN, OUTPUT);    // Initialize digital pin LED_BUILTIN as an output (used as ESP heartbeat indicator).
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (LOW) or off (HIGH) as needed. (NOTE: It is inverted.)
  Serial.begin(115200);            // Serial Communication baudrate: 9600, 115200, 250000

  //-- setup the singleton EspClient instance
  espClient.setup();
}

void loop()
{
  espClient.loop(); // task handling
}
