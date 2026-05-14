#pragma once

#include <Arduino.h>

constexpr uint32_t measurements_buffer_size = 20;

extern volatile int32_t measurements[];
extern volatile uint32_t intervals[];

extern volatile uint32_t last_measurement_idx;
extern volatile uint32_t measurements_count;

extern volatile uint64_t totalCharge;

extern uint32_t tsStart;

bool adcInit();
void adcConfig();
void adcProcessPendingData();
void adcResetData();
bool adcCalibrateZeroOffset(uint16_t sampleCount = 128);
