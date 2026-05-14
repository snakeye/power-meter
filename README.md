# Power Meter

Power Meter is a microcontroller-based current consumption meter.
It samples current with an external ADC, computes rolling values in firmware, and displays live readings on a small OLED screen.

This repository is organized around two parts:
- `firmware/` - PlatformIO project with STM32 firmware
- `hardware/` - KiCad schematic/PCB and symbol/footprint libraries

## Project status

This is an early public version. Core measurement and display flow is implemented, while some features are still work in progress.

Implemented:
- ADC continuous sampling using ADS1220 DRDY interrupt
- Raw-to-current conversion (microamps)
- On-screen display of averaged current, elapsed time, and accumulated charge estimate
- Reset of measurement session from button A

Not implemented yet / needs refinement:
- Persistent configuration load/save (`loadConfig`, `saveConfig` are currently stubs)
- Button B action (handler is empty)
- Charge accumulation algorithm validation and unit correctness improvements

## Quick start

Prerequisites:
- PlatformIO CLI installed
- ST-Link connected to target board

Build firmware:

```bash
cd firmware
platformio run
```

Flash firmware:

```bash
cd firmware
platformio run -t upload
```

Current default PlatformIO environment targets STM32G431C8 (`STM32G431C8T6`).

## Documentation

Use these documents as the main reference:
- `docs/overview.md` - project overview and data flow
- `docs/architecture.md` - system architecture and runtime model
- `docs/firmware.md` - firmware modules and behavior
- `docs/hardware.md` - hardware components and repository artifacts
- `docs/build-and-flash.md` - setup, build, flash, and debug workflow
- `docs/roadmap.md` - known issues and next milestones

## Notes for contributors

- Prefer documenting real current behavior over intended behavior.
- Keep units explicit (`uA`, `mA`, `mAh`, `us`, `ms`) in code and docs.
- If you change measurement math, update `docs/firmware.md` accordingly.
