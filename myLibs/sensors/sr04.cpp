#include "sr04.hpp"

SR04::SR04(const char *name, uint8_t triggerPin, uint8_t echoPin)
    : Sensor(name, 1, Median)
{
    _triggerPin = triggerPin;
    _echoPin = echoPin;
}

// _timestamp is the one of the lastest measure
char *SR04::getPayload()
{
  static char payload[50];
  // According to the C standard, unless the buffer size is 0, vsnprintf() and
  // snprintf() null terminates its output. No need to add \0 in its format
  // snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"distance\":%.1f}",
  //   _timestamp*1000, _measures[0]);

  if (!_bands[0]->status)
    return NULL;

  snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"distance\":%.1f}", _timestamp, _measures[0]);
  return payload;
}

// Measure distance in mm
// airTemp: the temperature in Celsius
bool SR04::_read()
{
  // _index is the current index of 5 measures.
  // Base class Sensor::measure() will shift the _index automatically

  static unsigned duration; // variable for the duration of sound wave travel
  static float soundSpeed;  // variable for the sound speed in the air

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
  pinMode(_echoPin, INPUT); // Sets the PIN_ECHO as an INPUT
  // You can use pulseIn with interrupts active, but results are less accurate.
  // ref: https://www.best-microcontroller-projects.com/arduino-pulsein.html
  noInterrupts();
  // For a different timeout use an unsigned long value (in microseconds) for the last parameter.
  duration = pulseIn(_echoPin, HIGH, 35000UL); // microseconds of (total) sound travel, 1e6 is 1s. timeout: default 1 second.
  interrupts();

  // Calculating the distance
  soundSpeed = 331300 + 606 * airTemp;        //  mm/s
  _measures[0] = soundSpeed * duration / 2e7; // cm // Speed of sound wave divided by 2 (go and back)
  // Serial.println(_lastReadings[0][_index]);

  return _measures[0] > 1e-3; // must be something, otherwise it is not connected
}
