#include "dht11.hpp"

// dhtPin: Digital pin connected to the DHT sensor
DH11::DH11(const char* name, uint8_t dhtPin)
	 : Sensor(name, 5), _dht(dhtPin, DHT11)
{
  _dht.begin();
}

// _timestamp is the one of the lastest measure
char* DH11::getPayload()
{
	static char payload[100];
	snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"humidity\":%.1f,\"temperature\":%.1f,\"heatindex\":%.1f}",
		_timestamp*1000,
		_measures[0],
		_measures[1],
		_measures[4]);
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
	
  _lastReadings[0][_index] = h;
	_lastReadings[1][_index] = t;
	_lastReadings[2][_index] = f;
	_lastReadings[3][_index] = hif;
	_lastReadings[4][_index] = hic;
	return true;
}