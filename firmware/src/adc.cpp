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

volatile uint32_t tsLastMeasurement = 0;
volatile int32_t measurements[measurements_buffer_size] = {0};
volatile uint32_t intervals[measurements_buffer_size] = {0};

volatile uint32_t last_measurement_idx = 0;
volatile uint32_t measurements_count = 0;
volatile uint64_t totalCharge = 0;
volatile uint32_t pendingDrdyCount = 0;
AdcCalibrationDiag lastCalibrationDiag = {false, false, false, 0, 0, 0, 0, 0};

void onAdcDataReadyInterrupt();

namespace
{
    void attachAdcInterrupt()
    {
        attachInterrupt(digitalPinToInterrupt(PIN_DRDY), onAdcDataReadyInterrupt, FALLING);
    }

    void detachAdcInterrupt()
    {
        detachInterrupt(digitalPinToInterrupt(PIN_DRDY));
    }

    bool applyRateModeToChip(uint8_t mode)
    {
        switch (mode)
        {
        case 0:
            ads.setDataRate(ADS1220_DR_LVL_6);
            return true;
        case 1:
            ads.setDataRate(ADS1220_DR_LVL_5);
            return true;
        case 2:
            ads.setDataRate(ADS1220_DR_LVL_4);
            return true;
        default:
            return false;
        }
    }
}

/**
 * ADC data ready interrup handler
 */
void onAdcDataReadyInterrupt()
{
    pendingDrdyCount += 1;
}

void adcProcessPendingData()
{
    constexpr uint32_t maxSamplesPerLoop = 4;

    for (uint32_t i = 0; i < maxSamplesPerLoop; i++)
    {
        noInterrupts();
        if (pendingDrdyCount == 0)
        {
            interrupts();
            break;
        }
        pendingDrdyCount -= 1;
        interrupts();

        int32_t adcValue = ads.getRawData() - config.adcZeroOffset;
        if (adcValue < 0)
        {
            adcValue = -adcValue;
        }

        int32_t current = int32_t(convertRawToCurrent_uA(adcValue));

        measurements[last_measurement_idx] = current;

        uint32_t now = micros();
        uint32_t tsDiff = now - tsLastMeasurement;

        intervals[last_measurement_idx] = tsDiff;

        last_measurement_idx = (last_measurement_idx + 1) % measurements_buffer_size;
        measurements_count += 1;
        tsLastMeasurement = now;

        totalCharge += uint64_t(current) * uint64_t(tsDiff);
    }
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
    attachAdcInterrupt();

    return true;
}

void adcConfig()
{
    ads.setCompareChannels(ADS1220_MUX_0_AVSS);
    ads.setVRefSource(ADS1220_VREF_INT);

    ads.setGain(ADS1220_GAIN_1);
}

void adcApplyConfig()
{
    if (!applyRateModeToChip(config.adcRateMode))
    {
        config.adcRateMode = 1;
        applyRateModeToChip(config.adcRateMode);
    }
}

bool adcSetRateMode(uint8_t mode)
{
    if (mode > 2)
    {
        return false;
    }

    if (config.adcRateMode == mode)
    {
        return true;
    }

    config.adcRateMode = mode;
    if (!applyRateModeToChip(config.adcRateMode))
    {
        config.adcRateMode = 1;
        applyRateModeToChip(config.adcRateMode);
        return false;
    }

    adcResetData();
    return true;
}

uint8_t adcGetRateMode()
{
    return config.adcRateMode;
}

void adcGetLastCalibrationDiag(AdcCalibrationDiag *out)
{
    if (out == nullptr)
    {
        return;
    }
    *out = lastCalibrationDiag;
}

void adcResetData()
{
    uint32_t nowMs = millis();
    uint32_t nowUs = micros();

    noInterrupts();

    tsStart = nowMs;

    for (size_t i = 0; i < measurements_buffer_size; i++)
    {
        measurements[i] = 0;
    }

    last_measurement_idx = 0;
    measurements_count = 0;
    pendingDrdyCount = 0;

    tsLastMeasurement = nowUs;

    totalCharge = 0;

    interrupts();
}

bool adcCalibrateZeroOffset(uint16_t sampleCount)
{
    lastCalibrationDiag.valid = true;
    lastCalibrationDiag.success = false;
    lastCalibrationDiag.timeout = false;
    lastCalibrationDiag.sampleCount = sampleCount;
    lastCalibrationDiag.minRaw = 0;
    lastCalibrationDiag.maxRaw = 0;
    lastCalibrationDiag.span = 0;
    lastCalibrationDiag.offset = config.adcZeroOffset;

    if (sampleCount == 0)
    {
        return false;
    }

    detachAdcInterrupt();

    int64_t sum = 0;
    int32_t minRaw = INT32_MAX;
    int32_t maxRaw = INT32_MIN;

    for (uint8_t i = 0; i < 16; i++)
    {
        (void)ads.getRawData();
    }

    for (uint16_t i = 0; i < sampleCount; i++)
    {
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

    attachAdcInterrupt();

    int32_t span = maxRaw - minRaw;
    lastCalibrationDiag.minRaw = minRaw;
    lastCalibrationDiag.maxRaw = maxRaw;
    lastCalibrationDiag.span = span;

    constexpr int32_t maxAllowedSpan = 5000;
    if (span > maxAllowedSpan)
    {
        return false;
    }

    config.adcZeroOffset = int32_t(sum / sampleCount);
    lastCalibrationDiag.offset = config.adcZeroOffset;
    lastCalibrationDiag.success = true;
    adcResetData();
    return true;
}
