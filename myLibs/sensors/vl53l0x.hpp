#pragma once

#include "sensor.hpp"
#include "Adafruit_VL53L0X.h"

/*
 * Sonar readings with median noise filtering
 */
class VL53L0X: public Sensor
{
  public:
    VL53L0X(const char* name);
    virtual char* getPayload();
      
  private:
  	bool _ready = false;
  	Adafruit_VL53L0X _lox = Adafruit_VL53L0X();
   
    virtual bool _read();
};
