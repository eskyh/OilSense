#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// struct CfgSensor
// {
//   char name[25];
//   char type[15];
//   uint8_t pin0=255; // 255 means not configured
//   uint8_t pin1=255;
//   uint8_t pin2=255;

// #ifdef _DEBUG
//   void print()
//   {
//     Serial.printf("Name\t: [%s]\n", name);
//     Serial.printf("Type\t: [%s]\n", type);
//     Serial.printf("Pin0\t: [%d]\n", pin0);
//     Serial.printf("Pin1\t: [%d]\n", pin1);
//     Serial.printf("Pin2\t: [%d]\n", pin2);
//   }
// #endif  
// };

// #define MAX_SENSORS 3

#define JSON_CAPACITY 1024

class Config
{
  // Singleton design (e.g., private constructor)
  public:
    static Config& instance();
    ~Config() {};

    bool loadConfig(const char *filename = "/config.json");
    bool saveConfig(const char *filename = "/config.json");

    // DynamicJsonDocument doc(JSON_CAPACITY);
    StaticJsonDocument<JSON_CAPACITY> doc;

    //------------------------------------------------
    // Module name
    String module;   // module name, used as mqttClientName, otaHost

    //Wifi
    String ssid;
    String pass;
    String ip;
    String gateway;

    // MQTT broker
    // char mqttClient[25];  // use the module name instead
    String mqttServer;
    uint16_t mqttPort = 1883;
    String mqttUser;
    String mqttPass;

    // OTA host name and pass
    // char otaHost[40]; // use module name instead
    String otaPass;

    // // Sensor config
    // CfgSensor sensors[MAX_SENSORS];
    // //------------------------------------------------

  // void printConfig(bool json=false);
  void printConfig();
#ifdef _DEBUG
  void printFile(const char *filename);
#endif

    uint8_t pinByName(String pin);
    // const char* nameByPin(uint8_t pin);

// protected:
//   void buildJson(StaticJsonDocument<JSON_CAPACITY> &doc);
//   void copyJson(StaticJsonDocument<JSON_CAPACITY> &doc);

private:
  // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
  Config() {};
  Config(const Config&) = delete; // deleting copy constructor.
  Config& operator=(const Config&) = delete; // deleting copy operator.
};