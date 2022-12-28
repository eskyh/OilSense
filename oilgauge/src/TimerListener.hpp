#pragma once

#include "main.hpp"
#include "StensTimer.h"
#include "EspClient.hpp"

// ----------------------------------------------------------
// NOTE: The OTA name is configured through the webserver
// And it is saved in the flash of the module which is the
// same as WiFi credentials. 
// ----------------------------------------------------------

//-----------------------------------------------------
// This function is called when a timer has passed
#define ACT_TICK      1
#define ACT_SENSOR    2

//-----------------------------------------------------
// Sensors
//-----------------------------------------------------

//-- HC-SR04
// Define the pin used of HC-SR04 ultrasonic sensor
//     These two can be set to the same pin while connecting trig/echo
// NOTE: The built-in LED is on pin D4, and it is inverted
//     using D4 will impact the built in LED

#include "sr04.hpp"
// SR04 sr04("SR04", D2, D2); // name, pinTrig, pinEcho
SR04 sr04("SR04", D1, D2); // name, pinTrig, pinEcho

//-- VL53L0X
// #include "vl53l0x.hpp"
// VL53L0X vl53("VL53"); // name. D1(SCL) and D2(SDA) has to be used

//-- DHT11
#include <Wire.h>
#include "dht11.hpp"
DH11 dh11("DH11", D5); // name, pinData

bool ssr_sr04 = true;
// bool ssr_vl53 = true;
bool ssr_dh11 = true;

//-----------------------------------------------------
// Timer related
//-----------------------------------------------------
/* stensTimer variable to be used later in the code */
StensTimer* pTimer = StensTimer::getInstance();
Timer* timer_sensor = NULL;
bool autoMode = true;
bool ledBlink = true;
bool doMeasure = false;

void measure()
{
  if(ssr_sr04) sr04.sendMeasure();
  // if(ssr_vl53) vl53.sendMeasure();
  if(ssr_dh11) dh11.sendMeasure();
}

void cmdHandler(const char* topic, const char* payload)
{
  // #ifdef _DEBUG
  //   // print the command received
  //   Serial.println(topic);
  //   Serial.println(payload);
  // #endif

  if(strcmp(topic, CMD_OPEN_PORTAL) == 0) // || // send by MQTT
    // strcmp(topic, "cmdOpenPortal") == 0)    // send by EspClient
  {
    #ifdef _DEBUG
      Serial.println(F("Open portal in cmdHandler()"));
    #endif

    EspClient::instance().openConfigPortal();

  }else if(strcmp(topic, CMD_SSR_FILTER) == 0)
  {
    int filter = atoi(payload);
    sr04.setFilter((FilterType)filter);
    // vl53.setFilter(filter);
    dh11.setFilter((FilterType)filter);
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
    if(timer_sensor!= NULL) pTimer->deleteTimer(timer_sensor);
    timer_sensor = pTimer->setInterval(ACT_SENSOR, atoi(payload)*1000);
  }else if(strcmp(topic, CMD_RESTART) == 0)
  {
    ESP.restart();
  }else if(strcmp(topic, CMD_SSR_SR04) == 0)
  {
    if(strcmp(payload, "true") == 0) ssr_sr04 = true;
    else ssr_sr04 = false;
//  }else if(strcmp(topic, CMD_SSR_VL53) == 0)
//  {
//    if(strcmp(payload, "true") == 0) ssr_vl53 = true;
//    else ssr_vl53 = false;
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

class TimerListener: public IStensTimerListener
{
  private:
    private:
    // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
    TimerListener() {};
    TimerListener(const TimerListener&);
    TimerListener& operator=(const TimerListener&);

    AsyncMqttClient &mqttClient = EspClient::instance().mqttClient;

    Timer *timer_sensor = NULL;

  public:
    static TimerListener& instance()
    {
      static TimerListener _instance;
      return _instance;
    };

    ~TimerListener() {};

    inline void setup()
    {
      //-- Set MQTT parameters for all sensors
      sr04.setMqtt(&mqttClient, MQTT_PUB_SR04, 0, false);
      // vl53.setMqtt(&mqttClient, MQTT_PUB_VL53, 0, false);
      dh11.setMqtt(&mqttClient, MQTT_PUB_DH11, 0, false);

      //-- Create timers and the callback function
      // pTimer->setStaticCallback(timerCallback);                // tell StensTimer which callback function to use
      pTimer->setInterval(this, ACT_TICK, 1e3);                   // every 1 second
      timer_sensor = pTimer->setInterval(this, ACT_SENSOR, 9e5);  // every 15min = 900 seconds. Save the returned timer for reset the time interval

      // client.setup(std::bind(&TimerListener::cmdHandler, this), MQTT_SUB_CMD);
      // client.setup(cmdHandler, MQTT_SUB_CMD);
    }

    inline virtual void timerCallback(Timer* timer)
    {
      /* check if the timer is one we expect */
      int action = timer->getAction();
      switch(action)
      {
        case ACT_TICK:
        {
          // send heartbeat message
          mqttClient.publish(MQTT_PUB_HEARTBEAT, PUB_QOS, true);

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
};