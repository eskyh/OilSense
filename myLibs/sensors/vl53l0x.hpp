#pragma once

#include "sensor.hpp"
#include "Adafruit_VL53L0X.h"

/*
 * Laser distance sensor using I2C
 */
class VL53L0X : public Sensor
{
public:
    VL53L0X(const char *name);
    virtual char *getPayload();

private:
    bool _ready = false;
    Adafruit_VL53L0X _lox = Adafruit_VL53L0X();

    virtual bool _read();
};
