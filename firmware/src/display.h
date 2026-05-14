#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

void displayInit();
void displayUpdate();
void displayShowStatus(const char *status, uint32_t durationMs = 1500);
void displayNotifyActivity();
void displaySetMenuState(bool active, uint8_t item, uint8_t rateMode);
