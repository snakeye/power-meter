## Hardware

Hardware design files are stored in `hardware/` and maintained in KiCad format.

## Main components

- ADS1220 - 24-bit external ADC for precision sampling
- INA213 - current-sense amplifier
- 0.1 Ohm shunt resistor (100 mOhm)
- STM32 microcontroller (firmware currently targets STM32G431C8)
- 128x64 I2C OLED display (SSD1306 compatible)
- Two user buttons (A/B)

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
