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
    char statusText[12] = "";
    uint32_t statusUntilMs = 0;
}

void displayInit()
{
    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFontMode(1);
}

void displayShowStatus(const char *status, uint32_t durationMs)
{
    strncpy(statusText, status, sizeof(statusText) - 1);
    statusText[sizeof(statusText) - 1] = '\0';
    statusUntilMs = millis() + durationMs;
}

void displayUpdate()
{
    uint32_t now = millis();
    uint32_t tdiff = now - tsStart;

    int hours = (tdiff / (1000 * 60 * 60)) % 24;
    int minutes = (tdiff / (1000 * 60)) % 60;
    int seconds = (tdiff / 1000) % 60;

    int64_t sum = 0;
    int32_t max = 0;

    size_t cnt = MIN(measurements_buffer_size, measurements_count);
    for (size_t i = 0; i < cnt; i++)
    {
        int32_t m = measurements[i];
        sum += m;
        if (m > max)
        {
            max = m;
        }
    }

    int64_t avg_v = (cnt > 0) ? (sum / cnt) : 0;

    u8g2.clearBuffer();

    // get current in microAmperes
    u8g2.setFont(u8g2_font_logisoso18_tr);

    //
    u8g2.setCursor(0, 20);
    int a = avg_v / 1000;
    int b = (avg_v / 100) % 10;
    u8g2.printf("%d.%d mA", a, b);

    //
    u8g2.setFont(u8g2_font_unifont_te);

    //

    u8g2.setCursor(0, 48);
    uint32_t deciMah = (uint32_t)(totalCharge / 360000000000ULL);
    uint32_t wholeMah = deciMah / 10;
    uint32_t fracMah = deciMah % 10;
    u8g2.printf("%lu.%01lu mAh", (unsigned long)wholeMah, (unsigned long)fracMah);

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
