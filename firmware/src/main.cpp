#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <ADS1220_WE.h>

#include <U8g2lib.h>

#include <OneButton.h>

#include <RecurringTask.h>

#define PIN_I2C_SDA PB7
#define PIN_I2C_SCL PB6

#define PIN_CS PA4
#define PIN_SCK PA5
#define PIN_MISO PA6
#define PIN_MOSI PA7
#define PIN_DRDY PB0

#define PIN_BTN_A PB1
#define PIN_BTN_B PB2

//
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Constants
const float r_shunt = 0.1;                   // Shunt resistance
const float v_ref = 2.048;                   // Reference voltage
const float adc_resolution = float(1 << 23); // 2^23 for the 24-bit ADC
const float gain = 50;

//
ADS1220_WE ads = ADS1220_WE(&SPI, PIN_CS, PIN_DRDY);
const uint32_t adcZeroOffset = 5360;

//
typedef U8G2_SSD1306_128X64_NONAME_F_HW_I2C Display;
Display u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/PIN_I2C_SCL, /* data=*/PIN_I2C_SDA);

// Buttons
OneButton btnA;
OneButton btnB;

unsigned long ts = 0;
const unsigned int measurements_buffer_size = 200;
volatile int32_t measurements[measurements_buffer_size] = {0};
volatile unsigned int last_measurement_idx = 0;
volatile int32_t measurements_count = 0;
volatile int64_t cumulative = 0;

void onDataReady()
{
    int32_t c = ads.getRawData() - adcZeroOffset;
    measurements[last_measurement_idx] = c;
    last_measurement_idx = (last_measurement_idx + 1) % measurements_buffer_size;
    cumulative += c;
    measurements_count += 1;
}

void resetData()
{
    ts = millis();
    for (size_t i = 0; i < measurements_buffer_size; i++)
    {
        measurements[i] = 0;
    }
    last_measurement_idx = 0;
    cumulative = 0;
    measurements_count = 0;
}

void onBtnAClick()
{
    resetData();
}

void onBtnBClick()
{
}

void setup()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    SPI.begin();

    // display
    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFontMode(1);
    u8g2.setFont(u8g2_font_unifont_te);

    // ADC
    ads.init();

    // ads.bypassPGA(true);
    ads.setCompareChannels(ADS1220_MUX_0_AVSS);

    ads.setGain(ADS1220_GAIN_1);
    ads.setOperatingMode(ADS1220_NORMAL_MODE);
    ads.setDataRate(ADS1220_DR_LVL_6);
    ads.setConversionMode(ADS1220_CONTINUOUS);
    ads.setDrdyMode(ADS1220_DRDY);

    attachInterrupt(PIN_DRDY, onDataReady, FALLING);

    // buttons
    btnA.setup(PIN_BTN_A, INPUT_PULLDOWN, false);
    btnA.attachClick(onBtnAClick);

    btnB.setup(PIN_BTN_B, INPUT_PULLDOWN, false);
    btnB.attachClick(onBtnBClick);
}

int64_t convertRawToCurrent_mA(int64_t rawValue)
{
    // Calculate the current in milliamperes using integer math
    float current = rawValue * ((v_ref * 1000.0f) / (adc_resolution * r_shunt * gain));
    return current;
}

void bzz(char *buf, int64_t val)
{
    int64_t val_c = convertRawToCurrent_mA(val);
    if (val_c > 1000)
    {
        int bufH = val_c / 1000;
        int bufL = val_c % 1000;
        sprintf(buf, "%d.%03d A", bufH, bufL);
    }
    else
    {
        sprintf(buf, "%d mA", val_c);
    }
}

void updateDisplay()
{
    uint32_t now = millis();
    uint32_t tdiff = now - ts;
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

    int32_t avg_v = cnt > 0 ? sum / cnt : 0;

    int32_t cum = convertRawToCurrent_mA(cumulative) / 3600;

    char buf[20] = {0};

    u8g2.clearBuffer();

    u8g2.setCursor(0, 12);
    bzz(buf, avg_v);
    u8g2.printf("I: %s", buf);

    u8g2.setCursor(0, 24);
    bzz(buf, max);
    u8g2.printf("M: %s", buf);

    // u8g2.setCursor(0, 36);

    // u8g2.printf("C: %s", buf);

    u8g2.setCursor(0, 64);
    u8g2.printf("%02d:%02d:%02d", hours, minutes, seconds);

    u8g2.sendBuffer();
}

void loop()
{
    btnA.tick();
    btnB.tick();

    RecurringTask::interval(100, updateDisplay);
}