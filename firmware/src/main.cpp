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

enum UiMode : uint8_t
{
    UI_MODE_RUN = 0,
    UI_MODE_MENU = 1,
};

enum MenuItem : uint8_t
{
    MENU_ITEM_RATE = 0,
    MENU_ITEM_CAL = 1,
    MENU_ITEM_EXIT = 2,
    MENU_ITEM_COUNT = 3,
};

UiMode uiMode = UI_MODE_RUN;
uint8_t menuItem = MENU_ITEM_RATE;

void updateMenuDisplay()
{
    displaySetMenuState(uiMode == UI_MODE_MENU, menuItem, adcGetRateMode());
}

void onBtnAClick()
{
    displayNotifyActivity();

    if (uiMode == UI_MODE_MENU)
    {
        menuItem = (menuItem + 1) % MENU_ITEM_COUNT;
        updateMenuDisplay();
        return;
    }

    adcResetData();
    displayShowStatus("RESET");
}

void onBtnBClick()
{
    displayNotifyActivity();

    if (uiMode != UI_MODE_MENU)
    {
        return;
    }

    switch (menuItem)
    {
    case MENU_ITEM_RATE:
    {
        uint8_t nextMode = (adcGetRateMode() + 1) % 3;
        if (adcSetRateMode(nextMode))
        {
            saveConfig();
            if (nextMode == 0)
            {
                displayShowStatus("RATE FAST");
            }
            else if (nextMode == 1)
            {
                displayShowStatus("RATE BAL");
            }
            else
            {
                displayShowStatus("RATE PREC");
            }
        }
        else
        {
            displayShowStatus("RATE ERR");
        }
        updateMenuDisplay();
        break;
    }
    case MENU_ITEM_CAL:
        if (adcCalibrateZeroOffset(128))
        {
            saveConfig();
            AdcCalibrationDiag diag = {};
            adcGetLastCalibrationDiag(&diag);

            char status[12] = {0};
            long span = (diag.span >= 0) ? diag.span : -diag.span;
            snprintf(status, sizeof(status), "O%ld S%ld", (long)diag.offset, span);
            uiMode = UI_MODE_RUN;
            updateMenuDisplay();
            displayShowStatus(status, 2500);
        }
        else
        {
            AdcCalibrationDiag diag = {};
            adcGetLastCalibrationDiag(&diag);

            char status[12] = {0};
            uiMode = UI_MODE_RUN;
            updateMenuDisplay();
            if (diag.timeout)
            {
                displayShowStatus("CAL TIME", 2500);
            }
            else
            {
                long span = (diag.span >= 0) ? diag.span : -diag.span;
                snprintf(status, sizeof(status), "NOISE %ld", span);
                displayShowStatus(status, 2500);
            }
        }
        break;
    case MENU_ITEM_EXIT:
    default:
        uiMode = UI_MODE_RUN;
        updateMenuDisplay();
        displayShowStatus("EXIT");
        break;
    }
}

void onBtnBLongPress()
{
    displayNotifyActivity();

    if (uiMode == UI_MODE_RUN)
    {
        uiMode = UI_MODE_MENU;
        menuItem = MENU_ITEM_RATE;
        updateMenuDisplay();
        displayShowStatus("MENU");
    }
    else
    {
        uiMode = UI_MODE_RUN;
        updateMenuDisplay();
        displayShowStatus("EXIT");
    }
}

void setup()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // display
    displayInit();
    displayNotifyActivity();

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
    adcApplyConfig();
    updateMenuDisplay();

    //
    adcResetData();
}

void loop()
{
    btnA.tick();
    btnB.tick();

    adcProcessPendingData();

    RecurringTask::interval(100, displayUpdate);
}
