
#include "Config.hpp"
#include <LittleFS.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

Config &Config::instance()
{
  static Config _instance;
  return _instance;
}

// Loads the configuration from a file
bool Config::loadConfig(const char *filename)
{
  if (!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return false;
  }

  if (!LittleFS.exists(filename))
  {
    Serial.print(F("FS: File doesn't exist: "));
    Serial.println(filename);
    return false;
  }

  // Open file for reading
  File file = LittleFS.open(filename, "r");

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Module name
  module = doc["module"].as<String>();
  Serial.printf("MODULE %s\n", module.c_str()); // module name, used as mqttClientName, otaHost

  // Wifi
  ssid = doc["wifi"]["ssid"].as<String>();
  pass = doc["wifi"]["pass"].as<String>();
  ip = doc["ip"].as<String>();
  gateway = doc["gateway"].as<String>();
  apPass = doc["appass"].as<String>();
  otaPass = doc["otapass"].as<String>();

  // MQTT broker
  mqttServer = doc["mqtt"]["server"].as<String>();
  mqttPort = doc["mqtt"]["port"];
  mqttUser = doc["mqtt"]["user"].as<String>();
  mqttPass = doc["mqtt"]["pass"].as<String>();

  // Close the file
  file.close();

#ifdef _DEBUG
  printFile(filename);
  printConfig();
#endif

  return true;
}

// Saves the configuration to a file
bool Config::saveConfig(const char *filename)
{
  if (!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return false;
  }

  // Delete existing file, otherwise it will be appended to the existing one
  if (LittleFS.exists(filename))
    LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, "w");
  if (!file)
  {
    Serial.println(F("Failed to create file"));
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();

#ifdef _DEBUG
  printConfig();
  printFile(filename);
#endif

  return false;
}

#ifdef _DEBUG

void Config::printConfig()
{
  Serial.println(F("----------------------------"));
  Serial.println(F("Configuration"));
  Serial.println(F("----------------------------"));

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

// Prints the content of a file to the Serial
void Config::printFile(const char *filename)
{
  if (!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return;
  }

  // Open file for reading
  File file = LittleFS.open(filename, "r");
  if (!file)
  {
    Serial.println(F("Failed to read file"));
    return;
  }

  Serial.println(F("----------------------------"));
  Serial.print(F("File: "));
  Serial.println(filename);
  Serial.println(F("----------------------------"));
  // Extract each characters by one by one
  while (file.available())
  {
    Serial.print((char)file.read());
  }
  Serial.println();
  Serial.println(F("----------------------------"));

  // Close the file
  file.close();
}
#endif