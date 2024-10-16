#include "dht11.hpp"

// dhtPin: Digital pin connected to the DHT sensor
DH11::DH11(const char* name, uint8_t dhtPin)
   : Sensor(name, 5, FT_None), _dht(dhtPin, DHT11)
{
  _dht.begin();
}

// Generate MQTT message payload based on current measurement
char* DH11::getPayload()
{
  static char payload[100];
  char tmp[30];
  // snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"humidity\":%.1f,\"temperature\":%.1f,\"heatindex\":%.1f}",
  // 	_timestamp*1000,
  // 	_measures[0],
  // 	_measures[1],
  // 	_measures[4]);

  if(!_bands[0]->status && !_bands[1]->status) return NULL;

  // snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"humidity\":%.1f,\"temperature\":%.1f}",
  //   _timestamp,
  // 	_measures[0],
  // 	_measures[1]);

  snprintf(payload, sizeof(payload), "{\"timestamp\":%lld", _timestamp);
  
  if(_bands[0]->status)
  {
    snprintf(tmp, sizeof(tmp), ",\"humidity\":%.1f", _measures[0]);
    strcat(payload, tmp);
  }

  if(_bands[1]->status)
  {
    snprintf(tmp, sizeof(tmp), ",\"temperature\":%.1f}", _measures[1]);
    strcat(payload, tmp);
  }

  return payload;
}

// Measure distance in mm
// airTemp: the temperature in Celsius
bool DH11::_read()
{
  // _index is the current index of 5 measures.
  // Base class Sensor::measure() will shift the _index automatically
  static float h, t, f;

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = _dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = _dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = _dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    #ifdef DEBUG
      Serial.println("Failed to read from DHT sensor!");
    #endif
    return false;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = _dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = _dht.computeHeatIndex(t, h, false);
  
  _measures[0] = h;
  _measures[1] = t;
  _measures[2] = f;
  _measures[3] = hif;
  _measures[4] = hic;

  return true;
}
