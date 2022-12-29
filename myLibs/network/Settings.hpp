#pragma once

#include <EEPROM.h>
#include <CRC32.h>

// Module configuration data structure in flash
struct Settings {
  //Wifi
  char ssid[25];
  char pass[15];
  char ip[16];

  // MQTT broker
  char mqttHost[25];
  int mqttPort;
  char mqttUser[20];
  char mqttPass[15];

  // OTA host name and pass
  char otaHost[40]; // this is also use as mqtt client name
  char otaPass[15];

  // Sensor config
  char sensors[40];

  // CRC used to check if the data is valid
  uint32_t CRC;

  Settings()
  {
		reset();
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
    
    // otaHost[0] = '\0';
		snprintf(otaHost, sizeof(otaHost), "%d", ESP.getChipId()); // initialize the otaHost name
    otaPass[0] = '\0';
    CRC = 0;
  }

  // use CRC to check EEPROM data
  // https://community.particle.io/t/determine-empty-location-with-eeprom-get-function/18869/12
  // bool getSettings()
  // {
  //     static bool valid = false; // record if the settings member varaible is valid

  //     if(!valid)
  //     {
  //         EEPROM.begin(sizeof(struct Settings));
  //         EEPROM.get(0, settings);

  //         uint32_t checksum = CRC32::calculate((uint8_t *)&settings, sizeof(settings)-sizeof(settings.CRC));
  //         valid = settings.CRC == checksum;

  //         // if CRC check fail, the data retrieved to settings are garbage, reset it.
  //         if(!valid)
  //         {
  //             settings.reset();
  //             Serial.println(F("Settings not initialized..."));
  //         }else
  //         {
  //         #ifdef _DEBUG
  //             Serial.println(F("\nCRC matched. Valid settings retrieved."));
  //             settings.print();
  //         #endif
  //         }
  //     }

  //     return valid;
  // }

  // void saveSettings()
  // {
  //   #ifdef _DEBUG
  //     Serial.println("Save settins to EEPROM.");
  //   #endif

  //   settings.CRC = CRC32::calculate((uint8_t *)&settings, sizeof(settings)-sizeof(settings.CRC));

  //   EEPROM.begin(sizeof(struct Settings));
  //   EEPROM.put(0, settings);
  //   EEPROM.commit();
  // }

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