#pragma once

#include <Arduino.h>

#define PIN_I2C_SDA PA8
#define PIN_I2C_SCL PA9

#define PIN_CS PA15
#define PIN_SCK PB3
#define PIN_MISO PB4
#define PIN_MOSI PB5
#define PIN_DRDY PB15

#define PIN_BTN_A PB7
#define PIN_BTN_B PB9

// Constants
constexpr int32_t shuntResistance = 100; // Shunt resistance, milliOhms
constexpr int32_t ina213gain = 50;       // Gain INA213

constexpr int32_t adcResolution = 1 << 23; // 2^23 for the 24-bit ADC, 8388608 counts.

//
struct Config
{
    int32_t referenceVoltage; // milliVolts
    int32_t adcZeroOffset;
    int32_t adcGain;
    uint8_t adcRateMode;
};

extern Config config;

bool loadConfig();
void saveConfig();
