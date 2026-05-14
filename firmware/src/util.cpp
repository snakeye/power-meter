#include <Arduino.h>

#include "config.h"
#include "util.h"

int64_t adcScale = int64_t(ina213gain) * int64_t(shuntResistance) * int64_t(adcResolution);

/**
 * Convert raw ADC value to microAmperes
 */
int64_t convertRawToCurrent_uA(int64_t rawValue)
{
    int64_t microAmps = (rawValue * int64_t(config.referenceVoltage) * 1000000) / (adcScale);
    return microAmps;
}

/**
 * Get microseconds as 64bit integer
 */
uint64_t get64bitMicros()
{
    static uint32_t microsOverflowCount = 0;
    static uint32_t previousMicros = 0;

    uint32_t currentMicros = micros();

    // Detect overflow
    if (currentMicros < previousMicros)
    {
        microsOverflowCount++;
    }

    previousMicros = currentMicros;

    // Combine overflow count and current micros to get 64-bit value
    return ((uint64_t)microsOverflowCount << 32) | currentMicros;
}