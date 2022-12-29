#pragma once

#include "sensor.hpp"

/*
 * Sonar readings with median noise filtering
 */
class SR04 : public Sensor
{
  public:
    SR04(const char* name, uint8_t triggerPin, uint8_t echoPin);
    virtual char* getPayload();
      
  private:
    // sensor
    uint8_t _triggerPin;
    uint8_t _echoPin;
    
    virtual bool _read();
};
