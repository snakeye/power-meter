## Overview

Power Meter is an embedded current measurement device built around an STM32 microcontroller, an ADS1220 external ADC, and an INA213 current-sense amplifier.

The firmware continuously samples current, stores recent values in a small ring buffer, computes an average for display, and presents runtime information on a 128x64 OLED.

## Goals

- Measure low current consumption with better resolution than basic MCU ADC paths.
- Provide simple on-device readout for quick bench testing.
- Keep the firmware compact and easy to iterate.

## Repository structure

- `firmware/` - PlatformIO project (Arduino framework on STM32)
- `hardware/` - KiCad project files (schematic, PCB, symbols, footprints)
- `docs/` - project documentation

## High-level data flow

1. ADS1220 performs continuous conversions.
2. DRDY pin triggers an interrupt on each new sample.
3. Interrupt handler reads raw ADC data and applies zero offset.
4. Raw counts are converted to current in microamps (`uA`).
5. Sample and interval are written to ring buffers.
6. UI task periodically renders average current, elapsed time, and charge estimate.

## Current maturity snapshot

Working now:
- ADC interrupt-driven sampling
- Current conversion and averaging
- OLED rendering loop
- Session reset on button A click

Still pending:
- EEPROM-backed config persistence API (`loadConfig` / `saveConfig`)
- Button B feature
- Charge integration model review for physical correctness
