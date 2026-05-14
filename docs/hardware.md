## Hardware

Hardware design files are stored in `hardware/` and maintained in KiCad format.

## Main components

- ADS1220 - 24-bit external ADC for precision sampling
- INA213 - current-sense amplifier
- 0.1 Ohm shunt resistor (100 mOhm)
- STM32 microcontroller (firmware currently targets STM32G431C8)
- 128x64 I2C OLED display (SSD1306 compatible)
- M24C02 external I2C EEPROM for config persistence
- Two user buttons (A/B)

## External EEPROM (M24C02)

Configuration persistence uses a discrete `M24C02` connected to the main I2C bus.

- Device type: `M24C02` (`2 Kbit = 256 bytes`)
- I2C base address: `0x50`
- Address pins: `A0/A1/A2 -> GND` (fixed address `0x50`)
- Typical page size: `8 bytes` (page write boundary must be respected)

Firmware write behavior:

- Data is split into 8-byte page writes.
- After each page write, firmware waits for device ready using ACK polling.
- Config data includes CRC32 to detect corruption.

Because OLED display shares the same I2C bus, EEPROM writes should remain infrequent (configuration or calibration events only).

## Measurement chain and equations

Current measurement path in this project:

1. Load current flows through shunt resistor.
2. Shunt creates a differential voltage proportional to current.
3. INA213 amplifies this shunt voltage.
4. ADS1220 digitizes amplified analog voltage.
5. Firmware converts ADC counts to current in `uA`.

Core equations:

- `Vshunt = Iload * Rshunt`
- `Vamp = GainINA * Vshunt = Iload * Rshunt * GainINA`
- `raw ~= (Vamp / Vref) * 2^23`
- `Iload = raw * Vref / (Rshunt * GainINA * 2^23)`

For the current firmware constants:

- `Rshunt = 0.1 Ohm`
- `GainINA = 50`
- `Vref = 2.048 V` (ADS1220 internal reference)
- `2^23 = 8,388,608`

Current per ADC count:

- `I_per_count = Vref / (Rshunt * GainINA * 2^23)`
- `I_per_count = 2.048 / (0.1 * 50 * 8,388,608) A`
- `I_per_count ~= 4.8828125e-8 A`
- `I_per_count ~= 48.828125 nA ~= 0.048828125 uA`

Counts per milliamp:

- `raw_per_mA = 0.001 / I_per_count`
- `raw_per_mA ~= 20,480 counts/mA`

Equivalent check values:

- `10 mA` -> about `204,800` counts
- `100 mA` -> about `2,048,000` counts
- `500 mA` -> about `10,240,000` counts (already above nominal 2^23 full scale and therefore limited by analog output swing and ADC range)

## Expected current range with INA213 at 3.3V

The practical upper current limit is usually set by INA213 output headroom before ADC full scale becomes the bottleneck.

Idealized upper bound using output rail as limit:

- `Imax_theory = Vout_max / (Rshunt * GainINA)`
- With `Vout_max = 3.3V`: `Imax_theory = 3.3 / (0.1 * 50) = 0.66A`

Real analog front-end usually cannot swing exactly to the rail. Practical range is therefore lower.

Recommended engineering expectation:

- Typical safe range: about `0 ... 0.55A`
- Practical observed limit for many boards: about `0.55 ... 0.65A`

Exact value should be validated on the assembled board and across temperature.

## Resolution vs real accuracy

The ADC quantization step is very small (`~48.8 nA/count`), but system accuracy is dominated by analog and calibration effects, not by ADC LSB.

Primary contributors:

- Shunt tolerance: `0.5%` scale error baseline.
- INA gain error and drift: additional scale and temperature-dependent error.
- Zero offset and drift: dominant at low currents.
- Noise (INA + ADS1220 + layout/EMI): causes short-term fluctuation.

With current firmware defaults, `adcZeroOffset = 5600` counts corresponds to approximately:

- `5600 * 0.048828125 uA ~= 273.44 uA`

This means that without robust zero calibration, low-current measurements can have large relative error.

## Practical accuracy expectations (before full calibration)

These are engineering expectations for this architecture, not guaranteed specs.

- `0.5 ... 5 mA`: often offset/noise limited, relative error can be high.
- `5 ... 100 mA`: generally usable, accuracy improves as signal dominates offset.
- `100 ... 600 mA`: usually dominated by scale error (shunt tolerance + gain error), often in low single-digit percent range after basic zeroing.

To tighten numbers, perform board-level calibration with known current loads.

## Charge integration units and conversion

Correct physical integration is:

- `dQ = I * dt`

If firmware stores:

- `I` in `uA`
- `dt` in `us`

then accumulated charge unit is:

- `Q` in `uA*us`

Conversion to milliamp-hours:

- `1 mAh = 1000 uA * 3600 s`
- `1 s = 1,000,000 us`
- `1 mAh = 1000 * 3600 * 1,000,000 = 3,600,000,000,000 uA*us`

Therefore:

- `mAh = Q_uA_us / 3,600,000,000,000`

When implemented this way, accumulated charge is correctly tied to measured current and elapsed time and does not break when ADC sample rate changes.

## Calibration recommendations

For robust measurements, use at least a 3-point procedure:

1. Zero-current calibration: record and store offset after warm-up.
2. Mid-scale calibration: validate gain around a representative working current.
3. High-scale calibration: validate near upper operating current without clipping.

Suggested firmware calibration outputs:

- `adcZeroOffset` (offset term)
- gain correction factor (multiplicative term)

Re-run calibration when changing shunt value, analog supply conditions, PCB revision, or thermal environment.

## Repository artifacts

Primary KiCad project files:
- `hardware/power-meter.kicad_pro`
- `hardware/power-meter.kicad_sch`
- `hardware/power-meter.kicad_pcb`

Libraries and custom symbols/footprints:
- `hardware/power-meter.kicad_sym`
- `hardware/INA213BIDCKT.kicad_sym`
- `hardware/STM32G431.kicad_sym`
- `hardware/power-meter.pretty/`

## Consistency note

Earlier project notes referenced STM32F103. Current firmware configuration and board definition target STM32G431C8.

If you are assembling hardware, verify the exact MCU variant against both:
- firmware board config (`firmware/platformio.ini`, `firmware/boards/genericSTM32G431C8.json`)
- KiCad schematic and BOM

## Suggested hardware documentation additions

For future public releases, consider adding:
- validated BOM with manufacturer part numbers
- board revision history
- calibration/test procedure
- connector and pinout diagrams
