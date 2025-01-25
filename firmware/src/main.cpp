#include <Arduino.h>
#include <Wire.h>

#include <U8g2lib.h>

#define PIN_I2C_SDA PB7
#define PIN_I2C_SCL PB6

typedef U8G2_SSD1306_128X64_NONAME_F_HW_I2C Display;

Display u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/PIN_I2C_SCL, /* data=*/PIN_I2C_SDA);

void setup()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFontMode(1);
}

void loop()
{
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_unifont_te);
    u8g2.setCursor(32, 32);
    u8g2.print("Hello");

    u8g2.sendBuffer();

    delay(1000);
}