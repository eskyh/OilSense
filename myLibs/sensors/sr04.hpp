#pragma once

#include "sensor.hpp"

/*
 * SR04 Ultrasonic distance sensor class
 */
class SR04 : public Sensor
{
public:
  SR04(const char *name, uint8_t triggerPin, uint8_t echoPin);
  virtual char *getPayload();

private:
  // sensor pins
  uint8_t _triggerPin;
  uint8_t _echoPin;

  virtual bool _read();
};
