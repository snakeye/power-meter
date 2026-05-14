#include <Arduino.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "adc.h"
#include "util.h"

// Display
typedef U8G2_SSD1306_128X64_NONAME_F_HW_I2C Display;

Display u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/PIN_I2C_SCL, /* data=*/PIN_I2C_SDA);

namespace
{
    constexpr uint32_t screenSaverTimeoutMs = 15000;
    constexpr uint8_t displayContrastAwake = 255;
    constexpr uint8_t displayContrastSaver = 0;

    char statusText[12] = "";
    uint32_t statusUntilMs = 0;
    uint32_t lastActivityMs = 0;
    bool screenSaverActive = false;
    bool menuActive = false;
    uint8_t menuItem = 0;
    uint8_t menuRateMode = 1;
}

void displayInit()
{
    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFontMode(1);
    u8g2.setContrast(displayContrastAwake);

    lastActivityMs = millis();
    screenSaverActive = false;
}

void displayNotifyActivity()
{
    lastActivityMs = millis();

    if (screenSaverActive)
    {
        screenSaverActive = false;
        u8g2.setContrast(displayContrastAwake);
    }
}

void displayShowStatus(const char *status, uint32_t durationMs)
{
    strncpy(statusText, status, sizeof(statusText) - 1);
    statusText[sizeof(statusText) - 1] = '\0';
    statusUntilMs = millis() + durationMs;
}

void displaySetMenuState(bool active, uint8_t item, uint8_t rateMode)
{
    menuActive = active;
    menuItem = item;
    menuRateMode = rateMode;
}

void displayUpdate()
{
    uint32_t now = millis();

    if (!screenSaverActive && (now - lastActivityMs) >= screenSaverTimeoutMs)
    {
        screenSaverActive = true;
        u8g2.setContrast(displayContrastSaver);
    }

    uint32_t tdiff = now - tsStart;

    int hours = (tdiff / (1000 * 60 * 60)) % 24;
    int minutes = (tdiff / (1000 * 60)) % 60;
    int seconds = (tdiff / 1000) % 60;

    int64_t sum = 0;
    int32_t max = 0;

    uint32_t safeMeasurementsCount = 0;
    uint64_t safeTotalCharge = 0;
    int32_t safeMeasurements[measurements_buffer_size] = {0};

    noInterrupts();
    safeMeasurementsCount = measurements_count;
    safeTotalCharge = totalCharge;
    size_t safeCnt = MIN(measurements_buffer_size, safeMeasurementsCount);
    for (size_t i = 0; i < safeCnt; i++)
    {
        safeMeasurements[i] = measurements[i];
    }
    interrupts();

    size_t cnt = MIN(measurements_buffer_size, safeMeasurementsCount);
    for (size_t i = 0; i < cnt; i++)
    {
        int32_t m = safeMeasurements[i];
        sum += m;
        if (m > max)
        {
            max = m;
        }
    }

    int64_t avg_v = (cnt > 0) ? (sum / cnt) : 0;

    u8g2.clearBuffer();

    if (menuActive)
    {
        const char *rateText = "BAL";
        if (menuRateMode == 0)
        {
            rateText = "FAST";
        }
        else if (menuRateMode == 2)
        {
            rateText = "PREC";
        }

        u8g2.setFont(u8g2_font_unifont_te);
        u8g2.setCursor(0, 14);
        u8g2.print("MENU");

        u8g2.setCursor(0, 32);
        if (menuItem == 0)
        {
            u8g2.printf(">RATE: %s", rateText);
        }
        else if (menuItem == 1)
        {
            u8g2.print(">CAL");
        }
        else
        {
            u8g2.print(">EXIT");
        }

        u8g2.setCursor(0, 64);
        u8g2.print("A:NEXT B:OK");
        u8g2.sendBuffer();
        return;
    }

    // get current in microAmperes
    u8g2.setFont(u8g2_font_logisoso18_tr);

    //
    char currentText[20] = {0};
    int a = avg_v / 1000;
    int b = (avg_v / 100) % 10;
    snprintf(currentText, sizeof(currentText), "%d.%d mA", a, b);
    int currentX = 128 - u8g2.getStrWidth(currentText);
    if (currentX < 0)
    {
        currentX = 0;
    }
    u8g2.setCursor(currentX, 20);
    u8g2.print(currentText);

    //
    u8g2.setFont(u8g2_font_unifont_te);

    //

    char chargeText[20] = {0};
    uint32_t deciMah = (uint32_t)(safeTotalCharge / 360000000000ULL);
    uint32_t wholeMah = deciMah / 10;
    uint32_t fracMah = deciMah % 10;
    snprintf(chargeText, sizeof(chargeText), "%lu.%01lu mAh", (unsigned long)wholeMah, (unsigned long)fracMah);
    int chargeX = 128 - u8g2.getStrWidth(chargeText);
    if (chargeX < 0)
    {
        chargeX = 0;
    }
    u8g2.setCursor(chargeX, 32);
    u8g2.print(chargeText);

    //
    u8g2.setCursor(0, 64);
    if (statusUntilMs != 0 && now < statusUntilMs)
    {
        u8g2.printf("%s", statusText);
    }
    else
    {
        u8g2.printf("%02d:%02d:%02d", hours, minutes, seconds);
    }

    //
    u8g2.sendBuffer();
}
