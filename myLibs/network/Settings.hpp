#pragma once

#include <CRC32.h>

// Module configuration data structure in flash
struct Settings {
  //Wifi
  char ssid[25] = "";
  char pass[15] = "";
  char ip[16] = "";

  // MQTT broker
  char mqttHost[25] = "";
  int mqttPort = 1883;
  char mqttUser[20] = "";
  char mqttPass[15] = "";

  // OTA host name and pass
  char otaHost[40] = ""; // this is also use as mqtt client name when needed
  char otaPass[15] = "";

  // CRC used to check if the data is valid
  uint32_t CRC = 0;

  Settings()
  {
    //  snprintf(otaHost, sizeof(otaHost), "ESP-%d", ESP.getChipId()); // initialize the otaHost name
    snprintf(otaHost, sizeof(otaHost), "E%d", ESP.getChipId()); // initialize the otaHost name
  }

  // Reset all parameters
  void reset()
  {
    ssid[0] = '\0';
    pass[0] = '\0';
    ip[0] = '\0';

    mqttHost[0] = '\0';
    mqttPort = 1883;
    mqttUser[0] = '\0';
    mqttPass[0] = '\0';
    
    otaHost[0] = '\0';
    otaPass[0] = '\0';
    CRC = 0;
  }

#ifdef _DEBUG
  void print()
  {
    Serial.println(F("-------------------"));
    Serial.print(F("SSID\t: ")); Serial.println(ssid);
    Serial.print(F("PASS\t: ")); Serial.println(pass);
    Serial.print(F("Stat IP\t: ")); Serial.println(ip);
    Serial.print(F("MQTT\t: ")); Serial.print(mqttHost); Serial.print(F(":")); Serial.println(mqttPort);
    Serial.print(F("user\t: ")); Serial.println(mqttUser);
    Serial.print(F("pass\t: ")); Serial.println(mqttPass);
    Serial.print(F("OTA\t: ")); Serial.println(otaHost);
    Serial.print(F("pass\t: ")); Serial.println(otaPass);
    Serial.println(F("-------------------"));
  }
#endif
};