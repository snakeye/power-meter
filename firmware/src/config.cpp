#include <Arduino.h>
#include <EEPROM.h>

#include "config.h"

Config config = {
    .referenceVoltage = 2048,
    .adcZeroOffset = 5600,
    .adcGain = 1,
};

struct B
{
    Config config;
    uint32_t crc;
};

bool loadConfig()
{
    return false;
}

void saveConfig()
{
}