
#include "Config.hpp"

#include <LittleFS.h>
// #include "SPIFFS.h"

// #include <map>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

Config& Config::instance()
{
    static Config _instance;
    return _instance;
}

// uint8_t Config::pinByName(String pin)
// {
//   if(pin == "D1") return D1;
//   else if(pin == "D2") return D2;
//   else if(pin == "D3") return D3;
//   else if(pin == "D4") return D4;
//   else if(pin == "D5") return D5;
//   else if(pin == "D6") return D6;
//   else if(pin == "D7") return D7;
//   else if(pin == "D8") return D8;
//   else if(pin == "RX") return RX;
//   else if(pin == "TX") return TX;
//   else { Serial.print("Wrong pin name: ");Serial.print(pin); return 255;}
// }

// const char* Config::nameByPin(uint8_t pin)
// {
//   static std::map<uint8_t, String> dict = {
//       {D1, "D1"},
//       {D2, "D2"},
//       {D3, "D3"},
//       {D4, "D4"},
//       {D5, "D5"},
//       {D6, "D6"},
//       {D7, "D7"},
//       {D8, "D8"},
//       {RX, "RX"},
//       {TX, "TX"},
//       {255, ""}}; // 255 means not configured

//   if(dict.find(pin) == dict.end()) return "";
//   else return dict[pin].c_str();
// }

// Loads the configuration from a file
// https://arduinojson.org/v6/example/config/
bool Config::loadConfig(const char *filename)
{
  if(!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return false;
  }

  if(!LittleFS.exists(filename))
  {
    Serial.print(F("FS: File doesn't exist: "));
    Serial.println(filename);
    return false;
  }

  // Open file for reading
  File file = LittleFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  // StaticJsonDocument<JSON_CAPACITY> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  // copyJson(doc);

  // Module name
  module = doc["module"].as<String>(); Serial.printf("MODULE %s\n", module.c_str()); // module name, used as mqttClientName, otaHost

  //Wifi
  ssid = doc["wifi"]["ssid"].as<String>();  // Serial.printf("SSID %s\n", ssid.c_str());
  pass = doc["wifi"]["pass"].as<String>();  // Serial.printf("PASS %s\n", pass.c_str());
  ip = doc["ip"].as<String>();              // Serial.printf("IP %s\n", ip.c_str());
  gateway = doc["gateway"].as<String>();    // Serial.printf("GATEWAY %s\n", gateway.c_str());
  apPass = doc["appass"].as<String>();
  otaPass = doc["otapass"].as<String>();

  // MQTT broker
  // char mqttClient[25];  // use the module name instead
  mqttServer = doc["mqtt"]["server"].as<String>(); // Serial.printf("MQTT %s\n", mqttServer.c_str());
  mqttPort = doc["mqtt"]["port"];                  // Serial.printf("MQTTPort %d\n", mqttPort);
  mqttUser = doc["mqtt"]["user"].as<String>();     // Serial.printf("M USER %s\n", mqttUser.c_str());
  mqttPass = doc["mqtt"]["pass"].as<String>();     // Serial.printf("M PASS %s\n", mqttPass.c_str());

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

 #ifdef _DEBUG
  printFile(filename);
  printConfig();
#endif

  return true;
}

// Saves the configuration to a file
// https://arduinojson.org/v6/example/config/
bool Config::saveConfig(const char *filename)
{
  if(!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return false;
  }

  // Delete existing file, otherwise the configuration is appended to the file
  if(LittleFS.exists(filename)) LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create file"));
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  // StaticJsonDocument<JSON_CAPACITY> doc;

  // Set the values in the document
  // buildJson(doc);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
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
  if(!LittleFS.begin())
  {
    Serial.println(F("FS: Failed to load json config"));
    return;
  }
  
  // Open file for reading
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  Serial.println(F("----------------------------"));
  Serial.print(F("File: ")); Serial.println(filename);
  Serial.println(F("----------------------------"));
  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  Serial.println(F("----------------------------"));

  // Close the file
  file.close();
}
#endif