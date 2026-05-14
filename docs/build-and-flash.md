## Build and Flash

This guide describes a minimal workflow for building and flashing firmware with PlatformIO.

## Prerequisites

- Python + PlatformIO CLI installed
- ST-Link probe connected
- Target board powered and wired

## Install PlatformIO CLI

If PlatformIO is not installed:

```bash
pip install platformio
```

## Build

From repository root:

```bash
cd firmware
platformio run
```

Build for a specific environment:

```bash
platformio run -e STM32G431C8T6
```

## Flash

```bash
cd firmware
platformio run -t upload
```

Flash a specific environment:

```bash
platformio run -e STM32G431C8T6 -t upload
```

## Serial monitor (optional)

If serial logging is added later:

```bash
platformio device monitor
```

## Debugging

The project is configured for ST-Link debug tool in `platformio.ini`.

Typical start:

```bash
platformio debug
```

## Troubleshooting

- Upload fails: verify SWD wiring and target power.
- Probe not found: check `stlink` connectivity and USB permissions.
- Wrong MCU behavior: confirm selected environment and board definition.
- Build mismatch: make sure local edits match `firmware/boards/genericSTM32G431C8.json`.
