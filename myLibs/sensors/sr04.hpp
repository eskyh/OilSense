#pragma once

#include "sensor.hpp"

/*
 * Sonar readings with median noise filtering
 */
class SR04 : public Sensor
{
  public:
    SR04(const char* name, int triggerPin, int echoPin);
    virtual char* getPayload();
      
  private:
    // sensor
    int _triggerPin;
    int _echoPin;
    
    virtual bool _read();
};
