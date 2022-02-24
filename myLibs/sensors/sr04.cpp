#include "sr04.hpp"

SR04::SR04(const char* name, int triggerPin, int echoPin)
	 : Sensor(name, 1)
{
  _triggerPin = triggerPin;
  _echoPin = echoPin;
}

// _timestamp is the one of the lastest measure
char* SR04::getPayload()
{
	static char payload[50];
  // According to the C standard, unless the buffer size is 0, vsnprintf() and
  // snprintf() null terminates its output. No need to add \0 in its format
  snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"distance\":%.1f}",
    _timestamp*1000, _measures[0]);
	return payload;
}

// Measure distance in mm
// airTemp: the temperature in Celsius
bool SR04::_read()
{
  // _index is the current index of 5 measures.
  // Base class Sensor::measure() will shift the _index automatically
  
  static unsigned duration;     // variable for the duration of sound wave travel
  static float soundSpeed;   // variable for the sound speed in the air
 
	float airTemp = 24.0;
	
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(_triggerPin, OUTPUT); // Sets the _triggerPin as an OUTPUT
  digitalWrite(_triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(_triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(_triggerPin, LOW);
  
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  // Reads the PIN_ECHO, returns the sound wave travel time in microseconds
  pinMode(_echoPin, INPUT);  // Sets the PIN_ECHO as an INPUT
  duration = pulseIn(_echoPin, HIGH, 35000); // microseconds of (total) sound travel. Timeout 35000 milliseconds

  // Calculating the distance
  soundSpeed = 331300 + 606 * airTemp; //  mm/s
  _lastReadings[0][_index] = soundSpeed*duration/2e7; //cm // Speed of sound wave divided by 2 (go and back)
  // Serial.println(_lastReadings[0][_index]);
  return true;
}
