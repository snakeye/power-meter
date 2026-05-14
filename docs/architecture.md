## Architecture

This project is split into hardware design artifacts and firmware runtime logic.

## Top-level components

- `hardware/` contains the electrical design in KiCad.
- `firmware/` contains embedded software for acquisition, processing, and UI.

## Firmware module map

- `src/main.cpp`
  - System setup
  - Button event wiring
  - Main loop tick handlers
- `src/adc.h`, `src/adc.cpp`
  - ADS1220 setup
  - DRDY interrupt handler
  - Measurement buffers and counters
  - Session reset and charge accumulator state
- `src/display.h`, `src/display.cpp`
  - OLED init (U8g2)
  - Periodic rendering of values
- `src/util.h`, `src/util.cpp`
  - ADC raw count to current conversion
  - 64-bit microsecond timestamp helper
- `src/config.h`, `src/config.cpp`
  - Runtime config structure and defaults
  - Persistence API placeholders (currently not implemented)

## Runtime model

### Setup phase

`setup()` performs initialization in this order:

1. I2C bus init
2. Display init
3. ADC init and ADC runtime configuration
4. Button A and B setup + click callbacks
5. Config load (currently stubbed)
6. Measurement data reset

### Main loop phase

`loop()` does not poll ADC directly. It:

- Advances button state machines (`btnA.tick()`, `btnB.tick()`).
- Schedules periodic display updates every 100 ms via `RecurringTask::interval(100, displayUpdate)`.

### Interrupt-driven acquisition

The ADS1220 DRDY pin is attached to `onAdcDataReadyInterrupt()`.

For each sample:

1. Read raw ADC data.
2. Apply configured zero offset.
3. Clamp negative values to zero.
4. Convert to current (`uA`).
5. Store current and time interval in fixed-size ring buffers.
6. Update counters/index and charge accumulator state.

## State ownership

Acquisition state is global and mostly held in `adc.cpp`:

- `measurements[]`, `intervals[]`
- `last_measurement_idx`, `measurements_count`
- `totalCharge`
- `tsStart`

Display logic reads this shared state to render user-visible metrics.

## Concurrency notes

- Measurements are updated in interrupt context and read in main-loop context.
- Several shared fields are marked `volatile`, but there is no explicit critical section around multi-field snapshots.
- As the project evolves, consider atomic snapshot/copy strategy for display calculations.

## Known architecture gaps

- Persistence layer exists only as API shape.
- Input model currently uses one active action (button A reset).
- Charge accumulation semantics require verification and likely redesign.
