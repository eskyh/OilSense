#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define JSON_CAPACITY 1024

class Config
{
  // Singleton design (e.g., private constructor)
  public:
    static Config& instance();
    ~Config() {};

    bool loadConfig(const char *filename = "/config.json");
    bool saveConfig(const char *filename = "/config.json");

    // Configuration Json doc object
    StaticJsonDocument<JSON_CAPACITY> doc;

    // Module name
    String module="Module-New";   // Module name used for MQTT client name, as well as OTA host name, e.g., OilGauge

    // WiFi settings
    String ssid;    // SSID
    String pass;    // Password
    String ip;      // Fixed IP if specified
    String gateway; // Gateway setting

    String apPass;  // SoftAP pass
    String otaPass; // OTA pass

    // MQTT broker settings
    String mqttServer;        // Broker IP or domain name
    uint16_t mqttPort = 1883; // Port, default to 1883
    String mqttUser;          // User
    String mqttPass;          // Password

  void printConfig();
#ifdef _DEBUG
  void printFile(const char *filename);
#endif

private:
  // Singleton design pattern required
  // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
  Config() {};
  Config(const Config&) = delete; // deleting copy constructor.
  Config& operator=(const Config&) = delete; // deleting copy operator.
};