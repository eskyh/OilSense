
#include "Config.hpp"

#include "LittleFS.h"
// #include "SPIFFS.h"

#include <map>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

Config& Config::instance()
{
    static Config _instance;
    static bool loaded = false;

    // do not put loadConfig in private constructor as it is not executed!
    if(!loaded)
    {
      _instance.loadConfig();
      loaded = true;
    }

    return _instance;
}

void Config::loadConfig()
{
  Serial.println(F("loadConfig()"));

  // clean FS, for testing
  // SPIFFS.format();

  if (LittleFS.begin())
  {
    Serial.println(F("FS: Mounting file system"));

    if (LittleFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println(F("FS: Reading config file"));
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile)
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc(1042);
        // StaticJsonDocument<1024> doc;

        DeserializationError error = deserializeJson(doc, buf.get());
        
        // Test if parsing succeeds.
        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }

        // copy string value to the settings
        const char *_module = doc["module"]; strncpy(module, _module, sizeof(module));
        const char *_ssid = doc["wifi"]["ssid"]; strncpy(ssid, _ssid, sizeof(ssid));
        const char *_pass = doc["wifi"]["pass"]; strncpy(pass, _pass, sizeof(pass));

        const char *_ip = doc["ip"]; strncpy(ip, _ip, sizeof(ip));
        const char *_gateway = doc["igatewayp"]; strncpy(gateway, _gateway, sizeof(gateway));
        
        const char *_mqttServer = doc["mqtt"]["server"]; strncpy(mqttServer, _mqttServer, sizeof(mqttServer));
        mqttPort = doc["mqtt"]["port"];
        const char *_mqttUser = doc["mqtt"]["user"]; strncpy(mqttUser, _mqttUser, sizeof(mqttUser));
        const char *_mqttPass = doc["mqtt"]["pass"]; strncpy(mqttPass, _mqttPass, sizeof(mqttPass));

        const char *_otaPass = doc["otapass"]; strncpy(otaPass, _otaPass, sizeof(otaPass));

        const JsonArray& jsSensors =  doc["sensors"];

        // for (auto sensor : sensors)
        for (int i=0, size=MIN(jsSensors.size(),MAX_SENSORS); i<size; i++)
        {
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
        // strncpy(mqttServer, doc["mqtt"]["host"].as<const char*>(), sizeof(module));
        // mqttPort = doc["mqtt"]["port"].as<uint8_t>();
      } 
      else {
        Serial.println(F("FS: Failed to load json config"));
      }

      configFile.close();
      Serial.println(F("FS: Closed file"));

      valid = true; // successfully loaded config file
    }else
    {
      Serial.println(F("FS: config.json does not exists"));  
    }

  } else
  {
    Serial.println(F("FS: failed mouning file system"));  
  }
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

void Config::saveConfig()
{
  Serial.println("Save Config to file.");

  print();

  DynamicJsonDocument doc(1024);
  // StaticJsonDocument<1024> doc;
  buildJson(doc);

  // File file = LittleFS.open("/config.json", "w");
  // serializeJson(doc, file);
  // file.close();
}

void Config::buildJson(DynamicJsonDocument &doc)
{
  doc["module"] = module;
  doc["ssid"] = ssid;
  doc["pass"] = pass;

  doc["ip"] = ip;
  doc["gateway"] = gateway;

  doc["mqtt"]["host"] = mqttServer;
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

        // nested["pin0"] = nameByPin(sensors[i].pin0);
        // nested["pin1"] = nameByPin(sensors[i].pin1);
        // nested["pin2"] = nameByPin(sensors[i].pin2);

        //serializeJson(array, Serial);
    }
  }

  doc["sensors"] = docSensors;
}

#ifdef _DEBUG
void Config::print(bool json)
{
  Serial.println(F("----------------------------"));
  if(json)
  {
    DynamicJsonDocument doc(1024);
    // StaticJsonDocument<1024> doc;
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
#endif