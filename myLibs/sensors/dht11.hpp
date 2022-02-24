#pragma once

#include "sensor.hpp"
#include "DHT.h"

/*
 * Sonar readings with median noise filtering
 */
class DH11 : public Sensor
{
  public:
    DH11(const char* name, uint8_t dhtPin);
    virtual char* getPayload();
      
  private:
    DHT _dht;
    
    virtual bool _read();
};
