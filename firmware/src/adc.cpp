#include <Arduino.h>
#include <SPI.h>
#include <ADS1220_WE.h>

#include "adc.h"
#include "config.h"
#include "util.h"

// ADS1220
SPIClass adsSPI(PIN_MOSI, PIN_MISO, PIN_SCK);
ADS1220_WE ads = ADS1220_WE(&adsSPI, PIN_CS, PIN_DRDY);

//
uint32_t tsStart = 0;

volatile uint64_t tsLastMeasurement = 0;
volatile int64_t measurements[measurements_buffer_size] = {0};
volatile uint32_t intervals[measurements_buffer_size] = {0};

volatile uint32_t last_measurement_idx = 0;
volatile uint32_t measurements_count = 0;
volatile uint64_t totalCharge = 0;

/**
 * ADC data ready interrup handler
 */
void onAdcDataReadyInterrupt()
{
    // get ADC reading
    int32_t adcValue = ads.getRawData() - config.adcZeroOffset;
    if (adcValue < 0)
        adcValue = 0;

    int64_t current = convertRawToCurrent_uA(adcValue);

    measurements[last_measurement_idx] = current;

    // get time interval between measurements
    uint64_t now = get64bitMicros();
    uint32_t tsDiff = now - tsLastMeasurement;

    intervals[last_measurement_idx] = tsDiff;

    // update counters
    last_measurement_idx = (last_measurement_idx + 1) % measurements_buffer_size;
    measurements_count += 1;
    tsLastMeasurement = now;

    // accumulate charge in uA*us
    totalCharge += uint64_t(current) * uint64_t(tsDiff);
}

bool adcInit()
{
    if (!ads.init())
    {
        return false;
    }
    ads.setAvddAvssAsVrefAndCalibrate();

    ads.bypassPGA(true);

    ads.setOperatingMode(ADS1220_NORMAL_MODE);

    ads.setConversionMode(ADS1220_CONTINUOUS);
    ads.setDrdyMode(ADS1220_DRDY);

    pinMode(PIN_DRDY, INPUT_PULLUP);
    attachInterrupt(PIN_DRDY, onAdcDataReadyInterrupt, FALLING);

    return true;
}

void adcConfig()
{
    ads.setCompareChannels(ADS1220_MUX_0_AVSS);
    ads.setVRefSource(ADS1220_VREF_INT);

    ads.setGain(ADS1220_GAIN_1);

    ads.setDataRate(ADS1220_DR_LVL_6);
}

void adcResetData()
{
    tsStart = millis();

    for (size_t i = 0; i < measurements_buffer_size; i++)
    {
        measurements[i] = 0;
    }

    last_measurement_idx = 0;
    measurements_count = 0;

    tsLastMeasurement = get64bitMicros();

    totalCharge = 0;
}

bool adcCalibrateZeroOffset(uint16_t sampleCount)
{
    if (sampleCount == 0)
    {
        return false;
    }

    detachInterrupt(PIN_DRDY);

    int64_t sum = 0;
    int32_t minRaw = INT32_MAX;
    int32_t maxRaw = INT32_MIN;

    auto waitForDataReady = []() -> bool
    {
        uint32_t startedAt = millis();
        while (digitalRead(PIN_DRDY) != LOW)
        {
            if ((millis() - startedAt) > 30)
            {
                return false;
            }
        }
        return true;
    };

    for (uint8_t i = 0; i < 16; i++)
    {
        if (!waitForDataReady())
        {
            attachInterrupt(PIN_DRDY, onAdcDataReadyInterrupt, FALLING);
            return false;
        }
        (void)ads.getRawData();
    }

    for (uint16_t i = 0; i < sampleCount; i++)
    {
        if (!waitForDataReady())
        {
            attachInterrupt(PIN_DRDY, onAdcDataReadyInterrupt, FALLING);
            return false;
        }

        int32_t raw = ads.getRawData();
        sum += raw;

        if (raw < minRaw)
        {
            minRaw = raw;
        }
        if (raw > maxRaw)
        {
            maxRaw = raw;
        }
    }

    attachInterrupt(PIN_DRDY, onAdcDataReadyInterrupt, FALLING);

    int32_t span = maxRaw - minRaw;
    constexpr int32_t maxAllowedSpan = 5000;
    if (span > maxAllowedSpan)
    {
        return false;
    }

    config.adcZeroOffset = int32_t(sum / sampleCount);
    adcResetData();
    return true;
}
