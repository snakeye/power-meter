#include <Arduino.h>
#include <Wire.h>

#include <U8g2lib.h>

#include <OneButton.h>

#include <RecurringTask.h>

#include "config.h"
#include "util.h"
#include "adc.h"
#include "display.h"

// Buttons
OneButton btnA;
OneButton btnB;

void onBtnAClick()
{
    adcResetData();
    displayShowStatus("RESET");
}

void onBtnBClick()
{
}

void onBtnBLongPress()
{
    if (adcCalibrateZeroOffset(128))
    {
        saveConfig();
        displayShowStatus("CAL SAVED", 2000);
    }
    else
    {
        displayShowStatus("CAL FAIL", 2000);
    }
}

void setup()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // display
    displayInit();

    // ADC
    adcInit();
    adcConfig();

    // buttons
    btnA.setup(PIN_BTN_A, INPUT_PULLDOWN, false);
    btnA.attachClick(onBtnAClick);

    btnB.setup(PIN_BTN_B, INPUT_PULLDOWN, false);
    btnB.attachClick(onBtnBClick);
    btnB.setPressMs(2000);
    btnB.attachLongPressStart(onBtnBLongPress);

    //
    loadConfig();

    //
    adcResetData();
}

void loop()
{
    btnA.tick();
    btnB.tick();

    RecurringTask::interval(100, displayUpdate);
}
