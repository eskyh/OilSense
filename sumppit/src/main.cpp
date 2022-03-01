#include "main.hpp"
#include "wifi.hpp"

// https://gitlab.com/arduino-libraries/stens-timer
#include <StensTimer.h>

// #include <ArduinoJson.h>

//-- Sensor HC-SR04
// ----------------------------------------------------
// Define the pin used of HC-SR04 ultrasonic sensor
//     These two can be set to the same pin while connecting trig/echo
// NOTE: The built-in LED is on pin D4, and it is inverted
//     using D4 will impact the built in LED
// ----------------------------------------------------
#include "sr04.hpp"
SR04 sr04("SR04", D5, D5); // name, pinTrig, pinEcho

//-- Sensor VL53L0X
#include "vl53l0x.hpp"
VL53L0X vl53("VL53"); // name. D1(SCL) and D2(SDA) has to be used

//-- Sensor DHT11
#include <Wire.h>
#include "dht11.hpp"
DH11 dh11("DH11", D7); // name, pinData

//--------------------------------------------------
// This function is called when a timer has passed
#define ACT_TICK      1
#define ACT_SENSOR    2

/* stensTimer variable to be used later in the code */
StensTimer* pStensTimer = NULL;
Timer* timer_sensor = NULL;
bool autoMode = true;
bool ledBlink = true;
bool doMeasure = false;

bool ssr_sr04 = true;
bool ssr_vl53 = true;
bool ssr_dh11 = true;

bool openCfgPortal = false;
// char reason[50]; // reason to open portal

void measure()
{
 if(ssr_sr04) sr04.sendMeasure();
 if(ssr_vl53) vl53.sendMeasure();
 if(ssr_dh11) dh11.sendMeasure();
}

//--------------------------------------------------
void cmdHandler(const char* topic, const char* payload)
{
  #ifdef _DEBUG
    // pring the command received
    Serial.println(topic);
    Serial.println(payload);
  #endif

  if(strcmp(topic, CMD_OPEN_PORTAL) == 0)
  {
    openCfgPortal = true;
  }else if(strcmp(topic, CMD_SSR_FILTER) == 0)
  {
    int filter = atoi(payload);
    sr04.setFilter(filter);
    vl53.setFilter(filter);
    dh11.setFilter(filter);
  }else if(strcmp(topic, CMD_LED_BLINK) == 0)
  {
    if(strcmp(payload, "true") == 0) ledBlink = true;
    else ledBlink = false;
  }else if(strcmp(topic, CMD_AUTO) == 0)
  {
    if(strcmp(payload, "true") == 0) autoMode = true;
    else autoMode = false;
  }else if(strcmp(topic, CMD_MEASURE) == 0)
  {
    // the actual measure is done in the TICKER task due to DHT11 reason
    doMeasure = true;
  }else if(strcmp(topic, CMD_INTERVAL) == 0)
  {
    if(timer_sensor!= NULL) pStensTimer->deleteTimer(timer_sensor);
    timer_sensor = pStensTimer->setInterval(ACT_SENSOR, atoi(payload)*1000);
  }else if(strcmp(topic, CMD_RESTART) == 0)
  {
    ESP.restart();
  }else if(strcmp(topic, CMD_SSR_SR04) == 0)
  {
    if(strcmp(payload, "true") == 0) ssr_sr04 = true;
    else ssr_sr04 = false;
  }else if(strcmp(topic, CMD_SSR_VL53) == 0)
  {
    if(strcmp(payload, "true") == 0) ssr_vl53 = true;
    else ssr_vl53 = false;
  }else if(strcmp(topic, CMD_SSR_DH11) == 0)
  {
    if(strcmp(payload, "true") == 0) ssr_dh11 = true;
    else ssr_dh11 = false;
  }else if(strcmp(topic, CMD_SLEEP) == 0)
  {
    if(strcmp(payload, "Modem") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Modem..."));
      #endif
      //  uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Modem");
    }
    else if(strcmp(payload, "Light") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Light..."));
      #endif
      // uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Light");
    }
    else if(strcmp(payload, "Deep") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Deep..."));
      #endif

      // After upload code, connect D0 and RST. NOTE: DO NOT connect the pins if using OTG uploading code!
//      String msg = "I'm awake, but I'm going into deep sleep mode for 60 seconds";
//      uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, msg.c_str());
//      delay(1000);
//      ESP.deepSleep(0); //10e6); 
    }
    else if(strcmp(payload, "Normal") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Normal..."));
      #endif
      // uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Normal");
    }
  }  
}


//--------------------------------------------------
void timerCallback(Timer* timer)
{
  /* check if the timer is one we expect */
  int action = timer->getAction();
  switch(action)
  {
    case ACT_TICK:
    {
      // send heartbeat message
      myWifi::mqttClient.publish(MQTT_PUB_HEARTBEAT, PUB_QOS, true);

      // manual measure flag
      // As mqtt call measure directly for DHT11 sensor (not others) will cause crash (see link below)
      // So the solution is to set a flag in mqtt and call for measure in the main loop.
      // https://forum.arduino.cc/t/dht11s-read-fine-from-loop-but-first-one-in-list-doesnt-work-when-in-timer/223383/5
      if(doMeasure)
      {
        measure();
        doMeasure = false; // reset the flag
      }

      // blink the built-in LED light
      if(ledBlink)
      {
        static bool led_on = true;
        digitalWrite(LED_BUILTIN, led_on ? LOW : HIGH); // turn the LED on (LOW) and off(HIGHT). It is inverted.
        led_on = !led_on;
      }else
      {
        digitalWrite(LED_BUILTIN, HIGH); // turn it off
      }
      break;
    }
      
    case ACT_SENSOR:
    {
      if(autoMode) measure();
      break;
    }
  }
}

void setup() {

  // initialize digital pin LED_BUILTIN as an output (used as heatbeat indicator of the board).
  pinMode(LED_BUILTIN, OUTPUT);
  // turn the LED on (LOW) and off(HIGH). It is inverted.
  digitalWrite(LED_BUILTIN, HIGH);  // turn off the LED
  
  Serial.begin(115200);      // Serial Communication baudrate: 9600, 115200, 250000

  myWifi::setupWifiListener();
  myWifi::autoConnect();

  // Set sensor mqtt parameters
  sr04.setMqtt(&(myWifi::mqttClient), MQTT_PUB_SR04, 0, false);
  vl53.setMqtt(&(myWifi::mqttClient), MQTT_PUB_VL53, 0, false);
  dh11.setMqtt(&(myWifi::mqttClient), MQTT_PUB_DH11, 0, false);
  
  // setup callback function for command topics
  myWifi::OnCommand(MQTT_SUB_CMD, cmdHandler); // setup command callback function

  // Save instance of StensTimer to the tensTimer variable
  pStensTimer = StensTimer::getInstance(); // Tell StensTimer which callback function to use
  pStensTimer->setStaticCallback(timerCallback);
  pStensTimer->setInterval(ACT_TICK, 1e3);                    // every 1 Second
  timer_sensor = pStensTimer->setInterval(ACT_SENSOR, 5e3);   // every 5 Second

  // disconnect WiFi when it's no longer needed
//  WiFi.disconnect(true);
//  WiFi.mode(WIFI_OFF);
}

void loop() {

  // let StensTimer do it's magic every time loop() is executed
  pStensTimer->run();

  // Give processing time for ArduinoOTA
  ArduinoOTA.handle();

  if(openCfgPortal)
  {
    myWifi::startConfigPortal();
    openCfgPortal = false;
  }
}
