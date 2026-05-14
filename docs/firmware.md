## Firmware

Firmware lives in `firmware/` and is built with PlatformIO using the Arduino framework for STM32.

## Build configuration

Main config file: `firmware/platformio.ini`

Current defaults:
- Platform: `ststm32`
- Framework: `arduino`
- Upload/debug: `stlink`
- Default env: `STM32G431C8T6`

Available environments:
- `STM32G431C8T6` (default)
- `bluepill` (legacy/alternate)

Custom board definition:
- `firmware/boards/genericSTM32G431C8.json`

## External libraries

- `ADS1220_WE` - ADS1220 ADC driver
- `U8g2` - OLED graphics
- `OneButton` - debounced button events
- `RecurringTask` - periodic task helper
- `Wire`, `SPI` - bus interfaces

## Module behavior

### `main.cpp`

- Initializes buses and peripherals.
- Registers button click handlers.
- Calls `loadConfig()` and `adcResetData()`.
- Runs button tick logic and display update scheduler in `loop()`.

Button actions:
- Button A click: resets current session data.
- Button B click: currently no action.

### `adc.cpp`

- Owns ADS1220 instance and SPI bus binding.
- Configures ADC for continuous conversion mode.
- Attaches falling-edge interrupt on DRDY.
- Converts raw ADC values to current via `convertRawToCurrent_uA()`.
- Stores values in fixed-size ring buffers (`measurements_buffer_size = 20`).

Data kept in global state:
- Samples: `measurements[]` in `uA`
- Per-sample delta: `intervals[]` in `us`
- Counters: `last_measurement_idx`, `measurements_count`
- Session start: `tsStart` in `ms`
- Accumulator: `totalCharge` (currently used as displayed charge estimate)

### `display.cpp`

- Uses `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`.
- Computes average current over available ring buffer entries.
- Renders:
  - average current in `mA` with one decimal
  - charge estimate in `mAh` (from `totalCharge`)
  - elapsed time as `HH:MM:SS`

Refresh cadence is controlled externally from `loop()` at 100 ms.

### `util.cpp`

- `convertRawToCurrent_uA(rawValue)` converts ADC counts to microamps using:
  - reference voltage (`config.referenceVoltage`, mV)
  - INA213 gain (`ina213gain`)
  - shunt resistance (`shuntResistance`, mOhm)
  - ADC resolution (`2^23`)
- `get64bitMicros()` extends Arduino `micros()` to a monotonic 64-bit timeline using overflow tracking.

### `config.cpp`

- Declares default runtime config values:
  - `referenceVoltage = 2048` mV
  - `adcZeroOffset = 5600`
  - `adcGain = 1`
- `loadConfig()` currently returns `false` and does not load EEPROM data.
- `saveConfig()` is empty.

## Pin mapping

Defined in `firmware/src/config.h`:

- I2C: `PA8` (SDA), `PA9` (SCL)
- ADS1220 SPI: `PA15` (CS), `PB3` (SCK), `PB4` (MISO), `PB5` (MOSI)
- ADS1220 DRDY: `PB15`
- Buttons: `PB7` (A), `PB9` (B)

## Known limitations

- Persistent configuration is not implemented.
- UI and ISR share state without explicit snapshot synchronization.
- Charge accumulation math should be reviewed and validated against target units.
- ADC error handling/recovery paths are minimal.
