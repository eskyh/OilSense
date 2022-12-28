#include "main.hpp"
#include "EspClient.hpp"
#include "TimerListener.hpp"

// https://gitlab.com/arduino-libraries/stens-timer
#include <StensTimer.h>

// #include <ArduinoJson.h>

// https://stackoverflow.com/questions/5459868/concatenate-int-to-string-using-c-preprocessor
// #define STR_HELPER(x) #x
// #define STR(x) STR_HELPER(x)

EspClient &espClient = EspClient::instance();
TimerListener &timerListener = TimerListener::instance();
// StensTimer* pStensTimer = StensTimer::getInstance();

void setup()
{
  //-- Initialize Pins and Serial speed

  // Initialize digital pin LED_BUILTIN as an output (used as ESP heartbeat indicator).
  pinMode(LED_BUILTIN, OUTPUT);
  // Turn the LED on (LOW) or off (HIGH) as needed. (NOTE: It is inverted.)
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Serial Communication baudrate: 9600, 115200, 250000
  Serial.begin(115200);

  timerListener.setup();
  espClient.setup(cmdHandler, MQTT_SUB_CMD); // cmdHandler() is defined in TimerListener.hpp
}

void loop()
{
  // NOTE: this has been taken acare of by EspClient class as SternsTimer is
  // singleton sharing one instance. It is included in EspClient.loop() which
  // in turn called in listener.loop() below.
  // // let StensTimer do it's magic every time loop() is executed
  // // pStensTimer->run(); 

  espClient.loop();
}
