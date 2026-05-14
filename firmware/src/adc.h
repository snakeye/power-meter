#pragma once

#include <Arduino.h>

constexpr uint32_t measurements_buffer_size = 20;

struct AdcCalibrationDiag
{
    bool valid;
    bool success;
    bool timeout;
    uint16_t sampleCount;
    int32_t minRaw;
    int32_t maxRaw;
    int32_t span;
    int32_t offset;
};

extern volatile int32_t measurements[];
extern volatile uint32_t intervals[];

extern volatile uint32_t last_measurement_idx;
extern volatile uint32_t measurements_count;

extern volatile uint64_t totalCharge;

extern uint32_t tsStart;

bool adcInit();
void adcConfig();
void adcApplyConfig();
void adcProcessPendingData();
void adcResetData();
bool adcCalibrateZeroOffset(uint16_t sampleCount = 128);
bool adcSetRateMode(uint8_t mode);
uint8_t adcGetRateMode();
void adcGetLastCalibrationDiag(AdcCalibrationDiag *out);
