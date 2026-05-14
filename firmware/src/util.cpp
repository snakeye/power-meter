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
