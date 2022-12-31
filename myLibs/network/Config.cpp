
#include "Config.hpp"

#include "LittleFS.h"
// #include "SPIFFS.h"

#include <map>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

Config& Config::instance()
{
    static Config _instance;
    return _instance;
}

uint8_t Config::pinByName(const char*pin)
{
  if(strcmp(pin, "D1") == 0) return D1;
  else if(strcmp(pin, "D2") == 0) return D2;
  else if(strcmp(pin, "D3") == 0) return D3;
  else if(strcmp(pin, "D4") == 0) return D4;
  else if(strcmp(pin, "D5") == 0) return D5;
  else if(strcmp(pin, "D6") == 0) return D6;
  else if(strcmp(pin, "D7") == 0) return D7;
  else if(strcmp(pin, "D8") == 0) return D8;
  else if(strcmp(pin, "RX") == 0) return RX;
  else if(strcmp(pin, "TX") == 0) return TX;
  else { Serial.print("Wrong pin name: ");Serial.print(pin); return 255;}
}

const char* Config::nameByPin(uint8_t pin)
{
  static std::map<uint8_t, String> dict = {
      {D1, "D1"},
      {D2, "D2"},
      {D3, "D3"},
      {D4, "D4"},
      {D5, "D5"},
      {D6, "D6"},
      {D7, "D7"},
      {D8, "D8"},
      {RX, "RX"},
      {TX, "TX"},
      {255, ""}}; // 255 means not configured

  if(dict.find(pin) == dict.end()) return "";
  else return dict[pin].c_str();
}

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

  printFile(filename);

  // Open file for reading
  File file = LittleFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<JSON_CAPACITY> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  copyJson(doc);
  // config.port = doc["port"] | 2731;
  // strlcpy(config.hostname,                  // <- destination
  //         doc["hostname"] | "example.com",  // <- source
  //         sizeof(config.hostname));         // <- destination's capacity

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
  StaticJsonDocument<JSON_CAPACITY> doc;

  // Set the values in the document
  buildJson(doc);

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

void Config::copyJson(StaticJsonDocument<JSON_CAPACITY> &doc)
{
  // copy string value to the settings
  const char *_module = doc["module"]; strncpy(module, _module, sizeof(module));
  const char *_ssid = doc["wifi"]["ssid"]; strncpy(ssid, _ssid, sizeof(ssid));
  const char *_pass = doc["wifi"]["pass"]; strncpy(pass, _pass, sizeof(pass));

  const char *_ip = doc["ip"]; strncpy(ip, _ip, sizeof(ip));
  const char *_gateway = doc["gateway"]; strncpy(gateway, _gateway, sizeof(gateway));
  
  const char *_mqttServer = doc["mqtt"]["server"]; strncpy(mqttServer, _mqttServer, sizeof(mqttServer));
  mqttPort = doc["mqtt"]["port"];
  const char *_mqttUser = doc["mqtt"]["user"]; strncpy(mqttUser, _mqttUser, sizeof(mqttUser));
  const char *_mqttPass = doc["mqtt"]["pass"]; strncpy(mqttPass, _mqttPass, sizeof(mqttPass));

  const char *_otaPass = doc["otapass"]; strncpy(otaPass, _otaPass, sizeof(otaPass));

  const JsonArray& jsSensors =  doc["sensors"];

  Serial.println("Copying sensors");

  // for (auto sensor : sensors)
  for (int i=0, size=MIN(jsSensors.size(),MAX_SENSORS); i<size; i++)
  {
    Serial.printf("Copying sensors %d\n", i);

    const char *_name = jsSensors[i]["name"]; strncpy(sensors[i].name, _name, sizeof(sensors[i].name));
    const char *_type = jsSensors[i]["type"]; strncpy(sensors[i].type, _type, sizeof(sensors[i].type));

    if(strcmp(_type, "HC-SR04") == 0)
    {
      sensors[i].pin0 = pinByName(jsSensors[i]["pinTrig"]);
      sensors[i].pin1 = pinByName(jsSensors[i]["pinEcho"]);

    }else if(strcmp(_type, "VL53L0X") == 0)
    {
      // no pin needed

    }else if(strcmp(_type, "DHT11") == 0)
    {
      sensors[i].pin0 = pinByName(jsSensors[i]["pinData"]);
    }else
    {
      Serial.print(F("Wrong sensor type: ")); Serial.println(_type);
    }
  }
  
  // check https://arduinojson.org/
  // strncpy(ip, doc["ip"].as<const char*>(), sizeof(ip));
  // strncpy(mqttServer, doc["mqtt"]["server"].as<const char*>(), sizeof(module));
  // mqttPort = doc["mqtt"]["port"].as<uint8_t>();
}

void Config::buildJson(StaticJsonDocument<JSON_CAPACITY> &doc)
{
  doc["module"] = module;
  
  doc["wifi"]["ssid"] = ssid;
  doc["wifi"]["pass"] = pass;

  doc["ip"] = ip;
  doc["gateway"] = gateway;

  doc["mqtt"]["server"] = mqttServer;
  doc["mqtt"]["port"] = mqttPort;
  doc["mqtt"]["user"] = mqttUser;
  doc["mqtt"]["pass"] = mqttPass;

  doc["otapass"] = otaPass;

  StaticJsonDocument<MAX_SENSORS*200> docSensors;
  JsonArray array = docSensors.to<JsonArray>();

  for(int i=0; i<MAX_SENSORS; i++)
  {
    if(sensors[i].type[0] != '\0')
    {
        JsonObject nested = array.createNestedObject();
        nested["name"] = sensors[i].name;
        nested["type"] = sensors[i].type;

        if(strcmp(sensors[i].type, "HC-SR04") == 0)
        {
          nested["pinTrig"] = nameByPin(sensors[i].pin0);
          nested["pinEcho"] = nameByPin(sensors[i].pin1);
        }else if(strcmp(sensors[i].type, "VL53L0X") == 0)
        {
          // no pin needed
        }else if(strcmp(sensors[i].type, "DHT11") == 0)
        {
          nested["pinData"] = nameByPin(sensors[i].pin0);
        }else
        {
          Serial.print(F("Wrong sensor type: ")); Serial.println(sensors[i].type);
        }
        //serializeJson(array, Serial);
    }
  }

  doc["sensors"] = docSensors;
}

#ifdef _DEBUG
void Config::printConfig(bool json)
{
  Serial.println(F("----------------------------"));
  Serial.println(F("Configuration"));
  Serial.println(F("----------------------------"));
  if(json)
  {
    // DynamicJsonDocument doc(JSON_CAPACITY);
    StaticJsonDocument<JSON_CAPACITY> doc;
    buildJson(doc);

    serializeJsonPretty(doc, Serial);
    Serial.println();
  }else
  {
    Serial.printf("Module\t: [%s]\n", module);
    Serial.printf("SSID\t: [%s]\n", ssid);
    Serial.printf("pass\t: [%s]\n", pass);
    Serial.printf("IP\t: [%s]\n", ip);
    Serial.printf("Gateway\t: [%s]\n", gateway);
    Serial.printf("MQTT\t: [%s:%d]\n", mqttServer, mqttPort);
    Serial.printf("user\t: [%s]\n", mqttUser);
    Serial.printf("pass\t: [%s]\n", mqttPass);
    Serial.printf("OTApass\t: [%s]\n", otaPass);

    for(int i=0; i<MAX_SENSORS; i++)
    {
      Serial.print("--Sensor "); Serial.println(i);
      sensors[i].print();
    }
  }
  Serial.println(F("----------------------------"));
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